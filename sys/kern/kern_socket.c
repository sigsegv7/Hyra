/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Hyra nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/socket.h>
#include <sys/sio.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <vm/dynalloc.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("socket: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static struct vops socket_vops;

/*
 * Get a kernel socket structure from a
 * file descriptor.
 *
 * @sockfd: File descriptor to lookup
 * @res: Result pointer
 *
 * Returns zero on success, otherwise a less
 * than zero errno.
 */
static int
get_ksock(int sockfd, struct ksocket **res)
{
    struct ksocket *ksock;
    struct filedesc *fdesc;
    struct vnode *vp;

    if (res == NULL) {
        return -EINVAL;
    }

    /* Grab the file descriptor */
    fdesc = fd_get(NULL, sockfd);
    if (fdesc == NULL) {
        return -EBADF;
    }

    /* Is this even a socket? */
    if ((vp = fdesc->vp) == NULL) {
        return -ENOTSOCK;
    }
    if (vp->type != VSOCK) {
        return -ENOTSOCK;
    }

    ksock = vp->data;
    if (__unlikely(ksock == NULL)) {
        return -EIO;
    }

    *res = ksock;
    return 0;
}

/*
 * VFS reclaim callback for the socket
 * layer
 *
 * Returns zero on success, otherwise a less
 * than zero errno.
 */
static int
socket_reclaim(struct vnode *vp)
{
    struct ksocket *ksock;

    /* Is this even a socket? */
    if (vp->type != VSOCK) {
        return -ENOTSOCK;
    }

    /* Is there any data attached? */
    if ((ksock = vp->data) == NULL) {
        return -EIO;
    }

    fd_close(ksock->sockfd);
    mutex_free(ksock->mtx);
    dynfree(ksock);
    return 0;
}

/*
 * Create a socket file from the sockaddr
 * structure
 *
 * @ksock: Socket to create a file for
 * @sockaddr_un: domain sockaddr
 */
static int
socket_mkfile(struct ksocket *ksock, struct sockaddr_un *un)
{
    struct filedesc *fdesc;
    struct vnode *vp;
    int fd;

    fd = fd_open(un->sun_path, O_CREAT | O_RDONLY);
    if (fd < 0) {
        return fd;
    }

    /* Grab the actual handle now */
    fdesc = fd_get(NULL, fd);
    if (fdesc == NULL) {
        fd_close(fd);
        return -EIO;
    }

    /* Hijack the vnode */
    vp = fdesc->vp;
    vp->type = VSOCK;
    vp->vops = &socket_vops;
    vp->data = ksock;
    return fd;
}

/*
 * Connect to a domain socket - used by connect()
 *
 * @sockfd: Socket file descriptor
 * @ksock: Current ksock
 * @un: Current sockaddr_un
 */
static int
connect_domain(int sockfd, struct ksocket *ksock, struct sockaddr_un *un)
{
    int error;
    struct nameidata ndp;
    struct filedesc *filedesc;
    struct vnode *vp;

    ndp.path = un->sun_path;
    ndp.flags = 0;
    if ((error = namei(&ndp)) < 0) {
        return error;
    }

    vp = ndp.vp;
    filedesc = fd_get(NULL, sockfd);
    if (filedesc == NULL) {
        pr_error("connect: no filedesc for current\n");
        return -EIO;
    }

    filedesc->vp = vp;
    return 0;
}

/*
 * Send data to socket - POSIX send(2) core
 *
 * @sockfd: File descriptor that backs this socket
 * @buf: Buffer containing data to transmit
 * @size: Size of the buffer
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less
 * than zero errno.
 */
ssize_t
send(int sockfd, const void *buf, size_t size, int flags)
{
    struct ksocket *ksock;
    struct sockbuf *sbuf;
    struct netbuf *netbuf;
    size_t tail;
    int error;

    /* Size cannot be zero */
    if (size == 0) {
        return -EINVAL;
    }

    if ((error = get_ksock(sockfd, &ksock)) < 0) {
        return error;
    }

    sbuf = &ksock->buf;
    netbuf = &sbuf->buf;
    mutex_acquire(ksock->mtx, 0);

    /* Make sure we dont overflow */
    if (netbuf->len > sbuf->watermark) {
        mutex_release(ksock->mtx);
        return -ENOBUFS;
    }

    if (netbuf->len == 0) {
        sbuf->head = 0;
        sbuf->tail = 0;
    }

    /* Clamp the size if needed */
    if ((netbuf->len + size) > sbuf->watermark) {
        size = sbuf->watermark - netbuf->len;
    }
    if (size == 0) {
        return -ENOBUFS;
    }

    /* Copy the new data */
    tail = sbuf->tail;
    memcpy(&netbuf->data[tail], buf, size);

    sbuf->tail += size;
    netbuf->len += size;
    mutex_release(ksock->mtx);
    return size;
}

/*
 * Recv data from socket - POSIX recv(2) core
 *
 * @sockfd: File descriptor that backs this socket
 * @buf: RX buffer
 * @size: Size of the buffer
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less
 * than zero errno.
 */
ssize_t
recv(int sockfd, void *buf, size_t len, int flags)
{
    struct ksocket *ksock;
    struct sockbuf *sbuf;
    struct netbuf *netbuf;
    size_t head;
    int error;

    /* Length cannot be zero */
    if (len == 0) {
        return -EINVAL;
    }

    if ((error = get_ksock(sockfd, &ksock)) < 0) {
        return error;
    }

    sbuf = &ksock->buf;
    netbuf = &sbuf->buf;
    mutex_acquire(ksock->mtx, 0);

    /* Is it empty? */
    if (netbuf->len == 0) {
        sbuf->head = 0;
        sbuf->tail = 0;
        return -EAGAIN;
    }

    if (len > netbuf->len) {
        len = netbuf->len;
    }

    head = sbuf->head;
    memcpy(buf, &netbuf->data[head], len);

    sbuf->head = (sbuf->head + len) % NETBUF_LEN;
    mutex_release(ksock->mtx);
    return len;
}

/*
 * POSIX socket(7) core
 *
 * @domain: Address family (see AF_*)
 * @type: Socket type
 * @protocol: Socket protocol
 *
 * Returns zero on success, otherwise a less
 * than zero errno.
 */
int
socket(int domain, int type, int protocol)
{
    struct ksocket *ksock = NULL;
    struct sockbuf *sbuf = NULL;
    struct proc *td = this_td();
    int fd, error = -1;

    ksock = dynalloc(sizeof(*ksock));
    if (ksock == NULL) {
        error = -ENOMEM;
        goto fail;
    }

    sbuf = &ksock->buf;
    sbuf->head = 0;
    sbuf->tail = 0;

    switch (domain) {
    case AF_UNIX:
        {
            struct sockaddr_un *un;

            un = &ksock->un;
            sbuf->watermark = NETBUF_LEN;

            /* Set up a path and create a socket file */
            un->sun_family = domain;
            snprintf(un->sun_path, sizeof(un->sun_path), "/tmp/%d-sock0", td->pid);
            fd = socket_mkfile(ksock, un);
        }
        return fd;
    default:
        error = -EINVAL;
        break;
    }

fail:
    if (ksock != NULL)
        dynfree(ksock);

    fd_close(fd);
    return error;
}

/*
 * Bind address to socket - POSIX bind(2) core
 *
 * @sockfd: File descriptor
 * @addr: Address to bind
 * @len: Sockaddr len
 *
 * Returns zero on success, otherwise a less
 * than zero errno.
 */
int
bind(int sockfd, const struct sockaddr *addr, socklen_t len)
{
    struct proc *td;
    struct ksocket *ksock;
    struct cmsg_list *clp;
    int error;

    if ((error = get_ksock(sockfd, &ksock)) < 0) {
        kprintf("error=%d\n", error);
        return error;
    }

    /* Create the new mutex lock */
    ksock->mtx = mutex_new("ksocket");
    if (ksock->mtx == NULL) {
        return -ENOMEM;
    }

    /* Mark ourselves as the owner */
    td = this_td();
    ksock->owner = td;

    /* Initialize the cmsg list queue */
    clp = &ksock->cmsg_list;
    TAILQ_INIT(&clp->list);
    clp->is_init = 1;
    return 0;
}

/*
 * Send socket control message - POSIX.1-2008
 *
 * @socket: Socket to transmit on
 * @msg: Further arguments
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less
 * than zero errno.
 */
ssize_t
sendmsg(int socket, const struct msghdr *msg, int flags)
{
    struct ksocket *ksock;
    struct cmsg *cmsg;
    struct sockaddr_un *un;
    struct cmsg_list *clp;
    size_t control_len = 0;
    int error;

    if ((error = get_ksock(socket, &ksock)) < 0) {
        return error;
    }

    /* We cannot do sendmsg() non domain sockets */
    un = &ksock->un;
    if (un->sun_family != AF_UNIX) {
        return -EBADF;
    }

    control_len = MALIGN(msg->msg_controllen);

    /* Allocate a new cmsg */
    cmsg = dynalloc(control_len + sizeof(struct cmsg));
    if (cmsg == NULL) {
        return -EINVAL;
    }

    memcpy(cmsg->buf, msg->msg_control, control_len);
    clp = &ksock->cmsg_list;
    cmsg->control_len = control_len;
    TAILQ_INSERT_TAIL(&clp->list, cmsg, link);
    return 0;
}

/*
 * Receive socket control message - POSIX.1‐2017
 *
 * @socket: Socket to receive on
 * @msg: Further arguments
 * @flags: Optional flags
 *
 * Returns zero on success, otherwise a less
 * than zero errno.
 */
ssize_t
recvmsg(int socket, struct msghdr *msg, int flags)
{
    struct ksocket *ksock;
    struct sockaddr_un *un;
    struct cmsg *cmsg, *tmp;
    struct cmsghdr *cmsghdr;
    struct cmsg_list *clp;
    uint8_t *fds;
    int error;

    if (socket < 0) {
        return -EINVAL;
    }

    /* Grab the socket descriptor */
    if ((error = get_ksock(socket, &ksock)) < 0) {
        return error;
    }

    /* Must be a unix domain socket */
    un = &ksock->un;
    if (un->sun_family != AF_UNIX) {
        return -EBADF;
    }

    /* Grab the control message list */
    clp = &ksock->cmsg_list;
    cmsg = TAILQ_FIRST(&clp->list);

    while (cmsg != NULL) {
        cmsghdr = &cmsg->hdr;

        /* Check the control message type */
        switch (cmsghdr->cmsg_type) {
        case SCM_RIGHTS:
            {
                fds = (uint8_t *)CMSG_DATA(cmsghdr);
                pr_trace("SCM_RIGHTS -> fd %d\n", fds[0]);
                break;
            }
        }

        tmp = cmsg;
        cmsg = TAILQ_NEXT(cmsg, link);

        TAILQ_REMOVE(&clp->list, tmp, link);
        dynfree(tmp);
    }

    return 0;
}

/*
 * socket(7) syscall
 *
 * arg0: domain
 * arg1: type
 * arg2: protocol
 */
scret_t
sys_socket(struct syscall_args *scargs)
{
    int domain = scargs->arg0;
    int type = scargs->arg1;
    int protocol = scargs->arg2;

    return socket(domain, type, protocol);
}

/*
 * bind(2) syscall
 *
 * arg0: sockfd
 * arg1: addr
 * arg2: len
 */
scret_t
sys_bind(struct syscall_args *scargs)
{
    const struct sockaddr *u_addr = (void *)scargs->arg1;
    struct sockaddr addr_copy;
    int sockfd = scargs->arg0;
    int len = scargs->arg2;
    int error;

    error = copyin(u_addr, &addr_copy, sizeof(addr_copy));
    if (error < 0) {
        return error;
    }

    return bind(sockfd, &addr_copy, len);
}

/*
 * recv(2) syscall
 *
 * arg0: sockfd
 * arg1: buf
 * arg2: size
 * arg3: flags
 */
scret_t
sys_recv(struct syscall_args *scargs)
{
    char buf[NETBUF_LEN];
    void *u_buf = (void *)scargs->arg1;
    int sockfd = scargs->arg0;
    size_t len = scargs->arg2;
    int error, flags = scargs->arg3;

    if (len > sizeof(buf)) {
        return -ENOBUFS;
    }

    error = recv(sockfd, buf, len, flags);
    if (error < 0) {
        pr_error("sys_recv: recv() fail (fd=%d)\n", sockfd);
        return error;
    }

    error = copyout(buf, u_buf, len);
    return (error == 0) ? len : error;
}

/*
 * send(2) syscall
 *
 * arg0: sockfd
 * arg1: buf
 * arg2: size
 * arg3: flags
 */
scret_t
sys_send(struct syscall_args *scargs)
{
    char buf[NETBUF_LEN];
    const void *u_buf = (void *)scargs->arg1;
    int sockfd = scargs->arg0;
    size_t len = scargs->arg2;
    int error, flags = scargs->arg3;

    if (len > sizeof(buf)) {
        return -ENOBUFS;
    }

    error = copyin(u_buf, buf, len);
    if (error < 0) {
        pr_error("sys_send: copyin() failure (fd=%d)\n", sockfd);
        return error;
    }

    return send(sockfd, buf, len, flags);
}

/*
 * recvmsg(3) syscall
 *
 * arg0: socket
 * arg1: msg
 * arg2: flags
 */
scret_t
sys_recvmsg(struct syscall_args *scargs)
{
    struct msghdr *u_msg = (void *)scargs->arg1;
    void *u_control;
    size_t controllen;
    struct iovec msg_iov;
    struct msghdr msg;
    ssize_t retval;
    int socket = scargs->arg0;
    int flags = scargs->arg2;
    int error;

    /* Read the message header */
    error = copyin(u_msg, &msg, sizeof(msg));
    if (error < 0) {
        pr_error("sys_recvmsg: bad msg\n");
        return error;
    }

    /* Grab the message I/O vector */
    error = uio_copyin(msg.msg_iov, &msg_iov, msg.msg_iovlen);
    if (error < 0) {
        return error;
    }

    /* Save control fields */
    u_control = msg.msg_control;
    controllen = msg.msg_controllen;

    /* Allocate a new control field to copy in */
    msg.msg_control = dynalloc(controllen);
    if (msg.msg_control == NULL) {
        uio_copyin_clean(&msg_iov, msg.msg_iovlen);
        return -ENOMEM;
    }

    error = copyin(u_control, msg.msg_control, controllen);
    if (error < 0) {
        retval = error;
        goto done;
    }

    msg.msg_iov = &msg_iov;
    retval = recvmsg(socket, &msg, flags);
done:
    uio_copyin_clean(&msg_iov, msg.msg_iovlen);
    dynfree(msg.msg_control);
    return retval;
}

/*
 * sendmsg(3) syscall
 *
 * arg0: socket
 * arg1: msg
 * arg2: flags
 */
scret_t
sys_sendmsg(struct syscall_args *scargs)
{
    struct iovec msg_iov;
    struct msghdr *u_msg = (void *)scargs->arg1;
    struct msghdr msg;
    ssize_t retval;
    int socket = scargs->arg0;
    int flags = scargs->arg2;
    int error;

    /* Read the message header */
    error = copyin(u_msg, &msg, sizeof(msg));
    if (error < 0) {
        pr_error("sys_sendmsg: bad msg\n");
        return error;
    }

    /* Grab the message I/O vector */
    error = uio_copyin(msg.msg_iov, &msg_iov, msg.msg_iovlen);
    if (error < 0) {
        return error;
    }

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &msg_iov;
    retval = sendmsg(socket, &msg, flags);
    uio_copyin_clean(&msg_iov, msg.msg_iovlen);
    return retval;
}

static struct vops socket_vops = {
    .read = NULL,
    .write = NULL,
    .reclaim = socket_reclaim,
};

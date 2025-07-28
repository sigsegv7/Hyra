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
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/filedesc.h>
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
    fdesc = fd_get(sockfd);
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
    struct vnode *vp = NULL;
    struct ksocket *ksock = NULL;
    struct filedesc *fdesc = NULL;
    struct sockbuf *sbuf = NULL;
    int fd, error = -1;

    if ((error = fd_alloc(&fdesc)) < 0) {
        return error;
    }

    fd = fdesc->fdno;

    /* Grab a new socket vnode */
    if ((error = vfs_alloc_vnode(&vp, VSOCK)) < 0) {
        fd_close(fd);
        return error;
    }

    if (vp == NULL) {
        error = -ENOBUFS;
        goto fail;
    }

    ksock = dynalloc(sizeof(*ksock));
    if (ksock == NULL) {
        error = -ENOMEM;
        goto fail;
    }

    fdesc->vp = vp;
    vp->vops = &socket_vops;

    sbuf = &ksock->buf;
    sbuf->head = 0;
    sbuf->tail = 0;

    switch (domain) {
    case AF_UNIX:
        {
            struct sockaddr_un *un;

            un = &ksock->un;
            sbuf->watermark = NETBUF_LEN;

            /*
             * XXX: We could allow actual paths within the
             *      file system for sockets.
             */
            un->sun_family = domain;
            un->sun_path[0] = '\0';
            vp->data = ksock;
        }
        return fd;
    default:
        error = -EINVAL;
        break;
    }

fail:
    if (ksock != NULL)
        dynfree(ksock);
    if (vp != NULL)
        vfs_release_vnode(vp);

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
    struct ksocket *ksock;
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
    return 0;
}

static struct vops socket_vops = {
    .read = NULL,
    .write = NULL,
    .reclaim = socket_reclaim,
};

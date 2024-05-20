/*
 * Copyright (c) 2023-2024 Ian Marco Moffett and the Osmora Team.
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

#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/sio.h>
#include <sys/sched.h>
#include <sys/errno.h>
#include <sys/system.h>
#include <sys/syslog.h>
#include <sys/vfs.h>
#include <sys/signal.h>
#include <sys/vnode.h>
#include <vm/dynalloc.h>
#include <dev/vcons/vcons.h>
#include <assert.h>
#include <string.h>

#define MAX_RW_SIZE  0x7FFFF000

/*
 * This function is a helper for write(). It creates
 * a buffer and copies write data to it.
 *
 * @td: Current thread.
 * @data: Data to copy.
 * @buf_out: Pointer to buffer that will store data.
 * @count: Number of bytes.
 */
static int
make_write_buf(struct proc *td, const void *data, char **buf_out, size_t count)
{
    char *buf = NULL;

    /* Count cannot be 0 or exceed the max size */
    if (count > MAX_RW_SIZE || count == 0) {
        return -EINVAL;
    }

    buf = dynalloc(count);

    if (buf == NULL) {
        return -ENOMEM;
    }

    __assert(buf_out != NULL);
    *buf_out = buf;

    memset(buf, 0, count);

    if (td->is_user) {
        /*
         * A user process called us, so we want to be careful
         * and use copyin()
         */
        if (copyin((uintptr_t)data, buf, count) != 0) {
            signal_raise(NULL, SIGSEGV);
        }
    } else {
        /* Can just memcpy() here */
        memcpy(buf, (char *)data, count);
    }

    return 0;
}

/*
 * Helper function for write()
 */
static ssize_t
do_write(struct vnode *vp, char *buf, size_t count)
{
    struct sio_txn sio = { .buf = buf, .len = count };
    struct vops *vops = vp->vops;
    int status;

    __assert(vops != NULL);

    /* Can we call the write operation? */
    if (vops->write == NULL) {
        return -EACCES;
    }

    /* Attempt a write */
    if ((status = vops->write(vp, &sio)) < 0) {
        return status;
    }

    return count;
}

/*
 * Allocate a file descriptor.
 *
 * @td: Thread to allocate from, NULL for current thread.
 * @fd_out: Pointer to allocated file descriptor output.
 *
 * This routine will create a new file descriptor
 * table entry.
 *
 * Returns 0 on success.
 */
int
fd_alloc(struct proc *td, struct filedesc **fd_out)
{
    struct filedesc *fd;

    if (td == NULL) {
        td = this_td();
        __assert(td != NULL);
    }

    /* Find free fd table entry */
    for (size_t i = 0; i < PROC_MAX_FDS; ++i) {
        if (td->fds[i] != NULL) {
            /* In use */
            continue;
        }

        fd = dynalloc(sizeof(struct filedesc));
        memset(fd, 0, sizeof(struct filedesc));

        if (fd == NULL) {
            return -ENOMEM;
        }

        fd->fdno = i;
        td->fds[i] = fd;

        if (fd_out != NULL)
            *fd_out = fd;

        return 0;
    }

    return -EMFILE;
}

/*
 * Fetch a file descriptor from a file descriptor
 * number.
 *
 * @td: Thread to fetch from, NULL for current thread.
 * @fdno: File descriptor to fetch
 */
struct filedesc *
fd_from_fdnum(const struct proc *td, int fdno)
{
    if (td == NULL) {
        td = this_td();
        __assert(td != NULL);
    }

    if (fdno < 0 || fdno > PROC_MAX_FDS) {
        return NULL;
    }

    for (size_t i = 0; i < PROC_MAX_FDS; ++i) {
        if (i == fdno && td->fds[i] != NULL) {
            return td->fds[i];
        }
    }

    return NULL;
}

/*
 * Close a file descriptor from its fd number.
 *
 * @td: Thread to fetch from, NULL for current thread.
 * @fdno: File descriptor number to close.
 */
void
fd_close_fdnum(struct proc *td, int fdno)
{
    struct filedesc *fd;

    if (td == NULL) {
        td = this_td();
        __assert(td != NULL);
    }

    fd = fd_from_fdnum(td, fdno);
    if (fd == NULL) {
        return;
    }

    dynfree(fd);
    td->fds[fdno] = NULL;
}

ssize_t
write(int fd, const void *buf, size_t count)
{
    struct proc *td = this_td();
    struct filedesc *desc = NULL;
    struct vnode *vp = NULL;
    char *in_buf = NULL;

    ssize_t ret = count;
    int status;

    /*
     * Create our write buffer... Memory will be allocated
     * and data copied.
     */
    if ((status = make_write_buf(td, buf, &in_buf, count)) != 0) {
        return status;
    }

    /* Is this stdout/stderr? */
    if (fd == 1 || fd == 2) {
        /* TODO: Update this when we have PTYs */
        vcons_putstr(&g_syslog_screen, in_buf, count);
        return count;
    }

    desc = fd_from_fdnum(td, fd);
    if (desc == NULL) {
        return -EBADF;
    }

    mutex_acquire(&desc->lock);
    if (desc->oflag != O_WRONLY && desc->oflag != O_WRONLY) {
        ret = -EACCES;
        goto cleanup;
    }

    /* Does this file descriptor exist? */
    if (desc == NULL) {
        ret = -EBADF;
        goto cleanup;
    }

    /* Do we have a vnode? */
    if (desc->vnode == NULL) {
        ret = -EACCES;
        goto cleanup;
    }

    vp = desc->vnode;
    status = do_write(vp, in_buf, count);

    if (status < 0) {
        ret = status;
        goto cleanup;
    }
cleanup:
    mutex_release(&desc->lock);
    dynfree(in_buf);
    return ret;
}

/*
 * Open a file and return a file descriptor.
 *
 * @pathname: File path.
 * @oflag: Flags.
 */
int
open(const char *pathname, int oflag)
{
    struct vnode *vp;
    struct filedesc *fd;
    int status;

    /*
     * Attempt to create a vnode, call the open hook then
     * allocate a file descriptor
     */
    if ((status = vfs_path_to_node(pathname, &vp)) != 0) {
        return status;
    }
    if ((status = vfs_open(vp)) != 0) {
        return status;
    }
    if ((status = fd_alloc(this_td(), &fd)) != 0) {
        return status;
    }

    fd->oflag = oflag;
    fd->vnode = vp;
    fd->is_dir = (vp->type == VDIR);
    return fd->fdno;
}

/*
 * Read file into a buffer.
 *
 * @fd: File descriptor number.
 * @buf: Buffer to read to.
 * @count: Number of bytes to read.
 */
int
read(int fd, void *buf, size_t count)
{
    ssize_t bytes_read;
    struct vnode *vnode;
    struct filedesc *fd_desc;
    struct sio_txn sio = {
        .buf = buf,
        .len = count,
        .type = SIO_NONE
    };

    fd_desc = fd_from_fdnum(this_td(), fd);
    if (fd_desc == NULL) {
        return -EBADF;
    }

    mutex_acquire(&fd_desc->lock);
    if (fd_desc->oflag == O_WRONLY) {
        bytes_read = -EACCES;
        goto done;
    }

    sio.offset = fd_desc->offset;
    vnode = fd_desc->vnode;

    if (count > MAX_RW_SIZE) {
        bytes_read = -EACCES;
        goto done;
    }

    bytes_read = vfs_read(vnode, &sio);
    fd_desc->offset += bytes_read;
done:
    mutex_release(&fd_desc->lock);
    return bytes_read;
}

/*
 * Reposition the file offset
 *
 * @fd: File descriptor.
 * @offset: Offset for the reposition
 * @whence: SEEK_SET, SEEK_CUR, or SEEK_END
 *
 * TODO: Implement SEEK_END
 */
off_t
lseek(int fd, off_t offset, int whence)
{
    struct filedesc *fd_desc;
    struct vattr vattr;

    fd_desc = fd_from_fdnum(this_td(), fd);

    if (fd_desc == NULL) {
        return -EBADF;
    }

    if (vfs_getattr(fd_desc->vnode, &vattr) != 0) {
        return -1;
    }

    switch (whence) {
    case SEEK_SET:
        if (offset > vattr.size)
            return -ESPIPE;

        fd_desc->offset = offset;
        break;
    case SEEK_CUR:
        if ((fd_desc->offset + offset) > vattr.size)
            return -ESPIPE;

        fd_desc->offset += offset;
        break;
    case SEEK_END:
        fd_desc->offset = vattr.size;
        break;
    default:
        return -EINVAL;
    }

    return fd_desc->offset;
}

/*
 * arg0: int fd
 * arg1: const void *buf
 * arg2: count
 */
uint64_t
sys_write(struct syscall_args *args)
{
    return write(args->arg0, (void *)args->arg1, args->arg2);
}

/*
 * arg0: const char *pathname
 * arg1: int oflag
 */
uint64_t
sys_open(struct syscall_args *args)
{
    char *pathbuf = dynalloc(sizeof(char) * PATH_MAX);
    int ret;

    if (pathbuf == NULL) {
        return -ENOMEM;
    }

    if (copyinstr(args->arg0, pathbuf, PATH_MAX) != 0) {
        signal_raise(NULL, SIGSEGV);
    }

    ret = open(pathbuf, args->arg1);
    dynfree(pathbuf);
    return ret;
}

/*
 * arg0: fd
 */
uint64_t
sys_close(struct syscall_args *args)
{
    fd_close_fdnum(this_td(), args->arg0);
    return 0;
}

/*
 * arg0: fd
 * arg1: char *buf
 * arg2: size_t count
 */
uint64_t
sys_read(struct syscall_args *args)
{
    char *kbuf;
    ssize_t bytes_read;

    if (args->arg2 > MAX_RW_SIZE || args->arg2 == 0) {
        return -EINVAL;
    }

    kbuf = dynalloc(args->arg2);
    if (kbuf == NULL) {
        return -ENOMEM;
    }

    /*
     * Try to read into our kernel buffer then copy out
     * to userspace.
     */
    if ((bytes_read = read(args->arg0, kbuf, args->arg2)) < 0) {
        /* Failure */
        dynfree(kbuf);
        return bytes_read;
    }
    if (copyout(kbuf, args->arg1, bytes_read) != 0) {
        signal_raise(NULL, SIGSEGV);
    }

    dynfree(kbuf);
    return bytes_read;
}

/*
 * arg0: fd
 * arg1: offset:
 * arg2: whence
 */
uint64_t
sys_lseek(struct syscall_args *args)
{
    return lseek(args->arg0, args->arg1, args->arg2);
}

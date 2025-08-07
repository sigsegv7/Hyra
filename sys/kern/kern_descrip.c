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

#include <sys/atomic.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/limits.h>
#include <sys/fcntl.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/systm.h>
#include <vm/dynalloc.h>
#include <string.h>

/*
 * Allocate a file descriptor.
 *
 * @td: Process to allocate from (null for CURRENT)
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
    }

    /* Find free fd table entry */
    for (size_t i = 3; i < PROC_MAX_FILEDES; ++i) {
        if (td->fds[i] != NULL) {
            /* In use */
            continue;
        }

        fd = dynalloc(sizeof(struct filedesc));
        memset(fd, 0, sizeof(struct filedesc));

        if (fd == NULL) {
            return -ENOMEM;
        }

        fd->refcnt = 1;
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
 * @td: Process to get fd from (NULL for current)
 * @fdno: File descriptor to fetch
 */
struct filedesc *
fd_get(struct proc *td, unsigned int fdno)
{
    if (td == NULL) {
        td = this_td();
    }

    if (fdno > PROC_MAX_FILEDES) {
        return NULL;
    }

    return td->fds[fdno];
}

/*
 * Close a file descriptor with a file
 * descriptor number.
 *
 * @fd: File descriptor number to close.
 */
int
fd_close(unsigned int fd)
{
    struct filedesc *filedes;
    struct proc *td;

    if ((filedes = fd_get(NULL, fd)) == NULL) {
        return -EBADF;
    }

    /* Return if other threads still hold a ref */
    if (atomic_dec_int(&filedes->refcnt) > 0) {
        return 0;
    }

    td = this_td();

    /*
     * Each file descriptor structure references a vnode,
     * we want to reclaim it or at the very least drop
     * one of its references. After we've cleaned up within
     * the file descriptor, we can clear it from the fd table
     * and free up the memory for it.
     */
    vfs_release_vnode(filedes->vp);
    td->fds[fd] = NULL;
    dynfree(filedes);
    return 0;
}

/*
 * Read/write bytes to/from a file using a file
 * descriptor number.
 *
 * @fd: File descriptor number.
 * @buf: Buffer with data to read/write
 * @count: Number of bytes to read.
 * @write: Set to 1 for writes
 */
static int
fd_rw(unsigned int fd, void *buf, size_t count, uint8_t write)
{
    char *kbuf = NULL;
    ssize_t n;
    uint32_t seal;
    struct filedesc *filedes;
    struct sio_txn sio;
    scret_t retval = 0;

    if (fd > PROC_MAX_FILEDES) {
        return -EBADF;
    }

    if (count > SSIZE_MAX) {
        retval = -EINVAL;
        goto done;
    }

    filedes = fd_get(NULL, fd);
    seal = filedes->flags;

    /* Check the seal */
    if (write && !ISSET(seal, O_ALLOW_WR)) {
        return -EPERM;
    }
    if (!write && ISSET(seal, O_WRONLY)) {
        return -EPERM;
    }

    kbuf = dynalloc(count);
    if (kbuf == NULL) {
        retval = -ENOMEM;
        goto done;
    }

    if (filedes == NULL) {
        retval = -EBADF;
        goto done;
    }

    if (filedes->is_dir) {
        retval = -EISDIR;
        goto done;
    }

    /* Check if this violates the file seal */
    if (!ISSET(filedes->flags, O_ALLOW_WR) && write) {
        return -EPERM;
    } else if (ISSET(O_RDONLY, filedes->flags) && write) {
        return -EPERM;
    }

    sio.len = count;
    sio.buf = kbuf;
    sio.offset = filedes->offset;

    spinlock_acquire(&filedes->lock);
    if (write) {
        /* Copy in user buffer */
        if (copyin(buf, kbuf, count) < 0) {
            retval = -EFAULT;
            goto done;
        }

        /* Call VFS write hook */
        if ((n = vfs_vop_write(filedes->vp, &sio)) < 0) {
            retval = n;
            goto done;
        }
    } else {
        if ((n = vfs_vop_read(filedes->vp, &sio)) < 0) {
            retval = n;
            goto done;
        }

        /* End of file? */
        if (n == 0) {
            retval = 0;
            goto done;
        }

        if (copyout(kbuf, buf, count) < 0) {
            retval = -EFAULT;
            goto done;
        }
    }

    /* Increment the offset per read */
    filedes->offset += n;
    retval = n;
done:
    if (kbuf != NULL) {
        dynfree(kbuf);
    }
    spinlock_release(&filedes->lock);
    return retval;
}

static int
fd_do_create(const char *path, struct nameidata *ndp)
{
    struct vop_create_args cargs;
    struct vnode *dirvp = ndp->vp;
    const struct vops *vops = dirvp->vops;
    int error;

    if (vops->create == NULL) {
        return -EINVAL;
    }

    cargs.path = path;
    cargs.ppath = ndp->path;
    cargs.dirvp = dirvp;
    cargs.vpp = &ndp->vp;
    if ((error = vops->create(&cargs)) < 0) {
        return error;
    }

    return 0;
}

int
fd_read(unsigned int fd, void *buf, size_t count)
{
    return fd_rw(fd, buf, count, 0);
}

int
fd_write(unsigned int fd, void *buf, size_t count)
{
    return fd_rw(fd, buf, count, 1);
}

/*
 * Open a file and get a file descriptor
 * number.
 *
 * @pathname: Path of file to open.
 * @flags: Flags to use.
 */
int
fd_open(const char *pathname, int flags)
{
    int error;
    const struct vops *vops;
    struct filedesc *filedes;
    struct nameidata nd;

    nd.path = pathname;
    nd.flags = ISSET(flags, O_CREAT) ? NAMEI_WANTPARENT : 0;

    if ((error = namei(&nd)) < 0) {
        return error;
    }

    if ((error = fd_alloc(NULL, &filedes)) != 0) {
        vfs_release_vnode(nd.vp);
        return error;
    }

    vops = nd.vp->vops;
    if (ISSET(flags, O_CREAT) && vops->create != NULL) {
        error = fd_do_create(pathname, &nd);
    }
    if (error < 0) {
        return error;
    }

    filedes->vp = nd.vp;
    filedes->flags = flags;
    return filedes->fdno;
}

/*
 * Duplicate a file descriptor. New file descriptor
 * points to the same vnode.
 *
 * @td: Process of fd to dup (NULL for current)
 * @fd: File descriptor to dup
 */
int
fd_dup(struct proc *td, int fd)
{
    int error;
    struct filedesc *new_desc, *tmp;

    if (td == NULL) {
        td = this_td();
    }

    tmp = fd_get(td, fd);
    if (tmp == NULL)
        return -EBADF;

    if ((error = fd_alloc(td, &new_desc)) != 0)
        return error;

    /* Ref that vnode before we point to it */
    vfs_vref(tmp->vp);
    new_desc->vp = tmp->vp;
    return new_desc->fdno;
}

off_t
fd_seek(int fildes, off_t offset, int whence)
{
    struct filedesc *tmp;
    struct vattr attr;
    struct vop_getattr_args getattr_args;

    tmp = fd_get(NULL, fildes);
    if (tmp == NULL) {
        return -EBADF;
    }

    getattr_args.vp = tmp->vp;
    getattr_args.res = &attr;
    if ((vfs_vop_getattr(&getattr_args)) < 0) {
        return -EPIPE;
    }

    switch (whence) {
    case SEEK_SET:
        tmp->offset = offset;
        break;
    case SEEK_CUR:
        tmp->offset += offset;
        break;
    case SEEK_END:
        tmp->offset = attr.size + offset;
        break;
    default:
        return -EINVAL;
    }

    return tmp->offset;
}

/*
 * Update file offset
 *
 * arg0: `filedes'
 * arg1: `offset'
 * arg2: `whence'
 */
scret_t
sys_lseek(struct syscall_args *scargs)
{
    return fd_seek(scargs->arg0, scargs->arg1, scargs->arg2);
}

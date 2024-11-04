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

#include <sys/atomic.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/limits.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/systm.h>
#include <vm/dynalloc.h>
#include <string.h>

/*
 * Allocate a file descriptor.
 *
 * @fd_out: Pointer to allocated file descriptor output.
 *
 * This routine will create a new file descriptor
 * table entry.
 *
 * Returns 0 on success.
 */
int
fd_alloc(struct filedesc **fd_out)
{
    struct filedesc *fd;
    struct proc *td = this_td();

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
 * @fdno: File descriptor to fetch
 */
struct filedesc *
fd_get(unsigned int fdno)
{
    struct proc *td = this_td();

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

    if ((filedes = fd_get(fd)) == NULL) {
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
 * Read bytes from a file using a file
 * descriptor number.
 *
 * @fd: File descriptor number.
 * @buf: Buffer to read into.
 * @count: Number of bytes to read.
 */
int
fd_read(unsigned int fd, void *buf, size_t count)
{
    char *kbuf = NULL;
    struct filedesc *filedes;
    struct sio_txn sio;
    scret_t retval = 0;

    if (count > SSIZE_MAX) {
        retval = -EINVAL;
        goto done;
    }

    filedes = fd_get(fd);
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

    sio.len = count;
    sio.buf = kbuf;
    sio.offset = filedes->offset;

    if ((count = vfs_vop_read(filedes->vp, &sio)) < 0) {
        retval = -EIO;
        goto done;
    }

    if (copyout(kbuf, buf, count) < 0) {
        retval = -EFAULT;
        goto done;
    }

    retval = count;
done:
    if (kbuf != NULL) {
        dynfree(kbuf);
    }
    return retval;
}

/*
 * Open a file and get a file descriptor
 * number.
 *
 * @pathname: Path of file to open.
 * @flags: Flags to use.
 *
 * TODO: Use of flags.
 */
int
fd_open(const char *pathname, int flags)
{
    int error;
    struct filedesc *filedes;
    struct nameidata nd;

    nd.path = pathname;
    nd.flags = 0;

    if ((error = namei(&nd)) < 0) {
        return error;
    }

    if ((error = fd_alloc(&filedes)) != 0) {
        vfs_release_vnode(nd.vp);
        return error;
    }

    filedes->vp = nd.vp;
    return filedes->fdno;
}

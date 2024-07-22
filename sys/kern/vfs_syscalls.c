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

#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/limits.h>
#include <sys/syslog.h>
#include <vm/dynalloc.h>
#include <assert.h>
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
static int
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

        fd->fdno = i;
        td->fds[i] = fd;

        if (fd_out != NULL)
            *fd_out = fd;

        return 0;
    }

    return -EMFILE;
}

/*
 * arg0: pathname
 * arg1: oflags
 *
 * TODO: Use oflags.
 */
scret_t
sys_open(struct syscall_args *scargs)
{
    int error;
    const char *pathname;
    char pathbuf[PATH_MAX];
    struct filedesc *filedes;
    struct nameidata nd;

    pathname = (char *)scargs->arg0;
    nd.path = pathbuf;
    nd.flags = 0;

    if (copyinstr(pathname, pathbuf, PATH_MAX) < 0) {
        return -EFAULT;
    }

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

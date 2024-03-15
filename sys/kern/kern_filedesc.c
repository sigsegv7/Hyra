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
#include <sys/sched.h>
#include <vm/dynalloc.h>
#include <assert.h>
#include <string.h>

/*
 * Allocate a file descriptor.
 *
 * @td: Thread to allocate from, NULL for current thread.
 */
struct filedesc *
fd_alloc(struct proc *td)
{
    struct filedesc *fd;

    if (td == NULL) {
        td = this_td();
        __assert(td != NULL);
    }

    for (size_t i = 0; i < PROC_MAX_FDS; ++i) {
        if (td->fds[i] != NULL) {
            continue;
        }

        fd = dynalloc(sizeof(struct filedesc));
        memset(fd, 0, sizeof(struct filedesc));

        if (fd == NULL) {
            return NULL;
        }

        fd->fdno = i;
        td->fds[i] = fd;
        return fd;
    }

    return NULL;
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

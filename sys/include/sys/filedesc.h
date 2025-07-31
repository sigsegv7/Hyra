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

#ifndef _SYS_FILEDESC_H_
#define _SYS_FILEDESC_H_

#include <sys/types.h>
#if defined(_KERNEL)
#include <sys/vnode.h>
#include <sys/syscall.h>
#include <sys/spinlock.h>
#include <sys/syscall.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct filedesc {
    int fdno;
    off_t offset;
    bool is_dir;
    int refcnt;
    int flags;
    struct vnode *vp;
    struct spinlock lock;
};

int fd_close(unsigned int fd);
int fd_read(unsigned int fd, void *buf, size_t count);
int fd_write(unsigned int fd, void *buf, size_t count);

int fd_alloc(struct proc *td, struct filedesc **fd_out);
int fd_open(const char *pathname, int flags);
off_t fd_seek(int fildes, off_t offset, int whence);

int fd_dup(struct proc *td, int fd);
struct filedesc *fd_get(struct proc *td, unsigned int fdno);

scret_t sys_lseek(struct syscall_args *scargs);

#endif  /* _KERNEL */
#endif  /* !_SYS_FILEDESC_H_ */

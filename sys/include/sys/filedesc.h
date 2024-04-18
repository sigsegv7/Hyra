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

#ifndef _SYS_FILEDESC_H_
#define _SYS_FILEDESC_H_

#include <sys/vnode.h>
#include <sys/spinlock.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define O_RDONLY 0x00000
#define O_WRONLY 0x00001
#define O_RDWR   0x00002

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

struct proc;

struct filedesc {
    int fdno;
    int oflag;
    off_t offset;
    bool is_dir;
    struct vnode *vnode;
    struct spinlock lock;
};

#if defined(_KERNEL)
int fd_alloc(struct proc *td, struct filedesc **fd_out);
struct filedesc *fd_from_fdnum(const struct proc *td, int fdno);
void fd_close_fdnum(struct proc *td, int fdno);
ssize_t write(int fd, const void *buf, size_t count);
int open(const char *pathname, int oflag);
int read(int fd, void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);

uint64_t sys_write(struct syscall_args *args);
uint64_t sys_open(struct syscall_args *args);
uint64_t sys_close(struct syscall_args *args);
uint64_t sys_read(struct syscall_args *args);
uint64_t sys_lseek(struct syscall_args *args);
#endif
#endif

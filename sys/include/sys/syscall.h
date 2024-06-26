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

#ifndef _SYS_SYSCALL_H_
#define _SYS_SYSCALL_H_

#include <sys/types.h>
#if defined(_KERNEL)
#include <machine/frame.h>
#endif

/* Do not reorder */
enum {
    SYS_exit = 1,
    SYS_write,
    SYS_open,
    SYS_close,
    SYS_read,
    SYS_lseek,
    SYS_mmap,
    SYS_munmap,
    SYS_ioctl,
    SYS_execv,
    SYS_mount,
    SYS_reboot,
    __MAX_SYSCALLS
};

struct syscall_args {
    uint64_t code;
    uint64_t arg0, arg1, arg2;
    uint64_t arg3, arg4, arg5, arg6;
    uint64_t ip;
    uint64_t sp;
};

#if defined(_KERNEL)
extern uint64_t(*g_syscall_table[__MAX_SYSCALLS])(struct syscall_args *args);
void __syscall(struct trapframe *tf);
#endif

#endif

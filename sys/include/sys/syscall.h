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

#ifndef _SYS_SYSCALL_H_
#define _SYS_SYSCALL_H_

#if !defined(__ASSEMBLER__)
#include <sys/types.h>
#include <sys/cdefs.h>
#if defined(_KERNEL) || defined(_OLIBC)
#include <machine/syscall.h>
#endif  /* _KERNEL || _OLIBC */
#endif

#define SYS_none    0
#define SYS_exit    1
#define SYS_open    2
#define SYS_read    3
#define SYS_close   4
#define SYS_stat    5
#define SYS_sysctl  6
#define SYS_write   7

#if defined(_KERNEL)
/* Syscall return value and arg type */
typedef ssize_t scret_t;
typedef ssize_t scarg_t;

struct syscall_args {
    scarg_t arg0;
    scarg_t arg1;
    scarg_t arg2;
    scarg_t arg3;
    scarg_t arg4;
    scarg_t arg5;
    scarg_t arg6;
    struct trapframe *tf;
};

extern const size_t MAX_SYSCALLS;
extern scret_t(*g_sctab[])(struct syscall_args *);
#endif  /* _KERNEL */

#endif  /* _SYS_SYSCALL_H_ */

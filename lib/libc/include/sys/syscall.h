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

#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H

#if !defined(__ASSEMBLER__)
#include <stdint.h>
#endif

#define SYS_exit  1
#define SYS_write 2
#define SYS_open  3
#define SYS_close 4
#define SYS_read  5
#define SYS_lseek 6
#define SYS_mmap  7
#define SYS_munmap 8
#define SYS_ioctl  9
#define SYS_mount  11

#if !defined(__ASSEMBLER__)
__attribute__((__always_inline__))
static inline long
syscall0(uint64_t code)
{
    volatile long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(code));
    return ret;
}

__attribute__((__always_inline__))
static inline long
syscall1(uint64_t code, uint64_t arg0)
{
    volatile long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0) : "memory");
    return ret;
}

__attribute__((__always_inline__))
static long inline
syscall2(uint64_t code, uint64_t arg0, uint64_t arg1)
{
    volatile long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1) : "memory");
    return ret;
}

__attribute__((__always_inline__))
static inline long
syscall3(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    volatile long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1), "d"(arg2) : "memory");
    return ret;
}

__attribute__((__always_inline__))
static inline long
syscall4(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    volatile long ret;
    register uint64_t _arg3 asm("r10") = arg3;
    asm volatile("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1), "d"(arg2), "r"(_arg3) : "memory");
    return ret;
}

__attribute__((__always_inline__))
static inline long
syscall5(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    volatile long ret;
    register uint64_t _arg3 asm("r10") = arg3;
    register uint64_t _arg4 asm("r9") = arg4;
    asm volatile("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1), "d"(arg2), "r"(_arg3), "r"(_arg4) : "memory");
    return ret;
}

__attribute__((__always_inline__))
static inline long
syscall6(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    volatile long ret;
    register uint64_t _arg3 asm("r10") = arg3;
    register uint64_t _arg4 asm("r9") = arg4;
    register uint64_t _arg5 asm("r8") = arg5;
    asm volatile("int $0x80" : "=a"(ret) : "a"(code), "D"(arg0), "S"(arg1), "d"(arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5) : "memory");
    return ret;
}

#define _SYSCALL_N(a0, a1, a2, a3, a4, a5, a6, name, ...) \
    name

#define syscall(...) \
_SYSCALL_N(__VA_ARGS__, syscall6, syscall5, \
            syscall4, syscall3, syscall2, syscall1, \
            syscall0)(__VA_ARGS__)
#endif  /* !defined(__ASSEMBLER__) */
#endif  /* !_SYS_SYSCALL_H */

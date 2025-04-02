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

#ifndef _MACHINE_SYSCALL_H_
#define _MACHINE_SYSCALL_H_

#if !defined(__ASSEMBLER__)
__always_inline static inline long
syscall0(uint64_t code)
{
    return 0;
}

__always_inline static inline long
syscall1(uint64_t code, uint64_t arg0)
{
    return 0;
}

__always_inline static long inline
syscall2(uint64_t code, uint64_t arg0, uint64_t arg1)
{
    return 0;
}

__always_inline static inline long
syscall3(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    return 0;
}

__always_inline static inline long
syscall4(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    return 0;
}

__always_inline static inline long
syscall5(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    return 0;
}

__always_inline static inline long
syscall6(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    return 0;
}

#define _SYSCALL_N(a0, a1, a2, a3, a4, a5, a6, name, ...) \
    name

#define syscall(...) \
_SYSCALL_N(__VA_ARGS__, syscall6, syscall5, \
            syscall4, syscall3, syscall2, syscall1, \
            syscall0)(__VA_ARGS__)

#endif  /* !__ASSEMBLER__ */
#endif  /* !_MACHINE_SYSCALL_H_ */

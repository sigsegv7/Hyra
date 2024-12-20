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

#ifndef _SYS_CDEFS_H_
#define _SYS_CDEFS_H_

#include <sys/param.h>

#define __ASMV          __asm__ __volatile__
#define __weak          __attribute__((__weak__))
#define __always_inline __attribute__((__always_inline__))
#define __packed        __attribute__((__packed__))
#define __dead          __attribute__((__noreturn__))
#define __unused        __attribute__((__unused__))
#define __nothing       ((void)0)
#define __likely(exp)   __builtin_expect(((exp) != 0), 1)
#define __unlikely(exp) __builtin_expect(((exp) != 0), 0)
#define __static_assert _Static_assert

#if defined(__cplusplus)
#define __BEGIN_DECLS   extern "C" {
#define __END_DECLS     }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#ifndef offsetof
#define offsetof(st, m) ((size_t)&(((st *)0)->m))
#endif  /* offsetof */

#if defined(_KERNEL)
/*
 *  Align data on a cache line boundary. This is
 *  mostly useful for certain locks to ensure they
 *  have their own cache line to reduce cache line
 *  bouncing.
 */
#ifndef __cacheline_aligned
#define __cacheline_aligned                             \
    __attribute__((__aligned__(COHERENCY_UNIT),         \
                __section__(".data.cacheline_aligned")))

#endif  /* __cacheline_aligned */
#endif  /* _KERNEL */

#endif  /* !_SYS_CDEFS_H_ */

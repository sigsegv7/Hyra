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

#ifndef _BITS__LOCK_H
#define _BITS__LOCK_H

#include <bits/_types.h>

typedef struct {
    volatile __uint8_t lock;
} __libc_spinlock_t;

#if defined(__x86_64__)
__attribute__((__always_inline__, __weak__)) inline int
__test_and_set(__libc_spinlock_t *lock)
{
    int tmp = 0;

    __asm__ __volatile__(
            "lock; cmpxchgb %b2, %0"
            : "+m" (lock->lock), "+a" (tmp)
            : "r" (1)
            : "cc"
    );

    return tmp;
}
#else
#warn "Unimplemented arch"
#endif

#define __spinlock_def(NAME) __libc_spinlock_t NAME = {0};
#define __spinlock_acquire(LOCKPTR) while (__test_and_set(LOCKPTR))
#define __spinlock_release(LOCKPTR) ((LOCKPTR)->lock = 0)

#endif  /* _BITS__LOCK_H */

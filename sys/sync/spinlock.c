/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

#include <sync/spinlock.h>

#if defined(__x86_64__)
void
spinlock_acquire(struct spinlock *lock)
{
    while (__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE));
}

void
spinlock_release(struct spinlock *lock)
{
    __atomic_clear(&lock->lock, __ATOMIC_RELEASE);
}
#elif defined(__aarch64__)
# include <sys/cdefs.h>

void
spinlock_acquire(struct spinlock *lock)
{
    uint32_t tmp;
    __asmv("    sevl\n"
           "1:  wfe\n"
           "2:  ldaxr %w0, [%1]\n"
           "    cbnz %w0, 1b\n"
           "    stxr %w0, %w2, [%1]\n"
           "    cbnz %w0, 2b\n"
           : "=&r" (tmp)
           : "r" (&lock->lock), "r" (1)
           : "memory"
    );
}

void
spinlock_release(struct spinlock *lock)
{
    __asm("    stlr %w1, [%0]\n"
          :
          : "r" (&lock->lock), "r" (0)
          : "memory"
    );
}

#endif

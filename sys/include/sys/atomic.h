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

#ifndef _SYS_ATOMIC_H_
#define _SYS_ATOMIC_H_

static inline unsigned long
atomic_add_long_nv(volatile unsigned long *p, unsigned long v)
{
    return __sync_add_and_fetch(p, v);
}

static inline unsigned int
atomic_add_int_nv(volatile unsigned int *p, unsigned int v)
{
    return __sync_add_and_fetch(p, v);
}

static inline unsigned long
atomic_sub_long_nv(volatile unsigned long *p, unsigned long v)
{
    return __sync_sub_and_fetch(p, v);
}

static inline unsigned int
atomic_sub_int_nv(volatile unsigned int *p, unsigned int v)
{
    return __sync_sub_and_fetch(p, v);
}

static inline unsigned int
atomic_load_int_nv(volatile unsigned int *p, unsigned int v)
{
    return __atomic_load_n(p, v);
}

static inline unsigned int
atomic_load_long_nv(volatile unsigned long *p, unsigned int v)
{
    return __atomic_load_n(p, v);
}

static inline void
atomic_store_int_nv(volatile unsigned int *p, int nv, unsigned int v)
{
    __atomic_store_n(p, nv, v);
}

static inline void
atomic_store_long_nv(volatile unsigned long *p, long nv, unsigned int v)
{
    __atomic_store_n(p, nv, v);
}

/* Atomic increment (and fetch) operations */
#define atoimc_inc_long(P) atomic_add_long_nv((P), 1)
#define atomic_inc_int(P) atomic_add_int_nv((P), 1)

/* Atomic decrement (and fetch) operations */
#define atomic_dec_long(P) atomic_sub_long_nv((P), 1)
#define atomic_dec_int(P) atomic_sub_int_nv((P), 1)

/* Atomic load operations */
#define atomic_load_int(P) atomic_load_int_nv((P), __ATOMIC_SEQ_CST)
#define atomic_load_long(P) atomic_load_long_nv((P), __ATOMIC_SEQ_CST)

/* Atomic store operations */
#define atomic_store_int(P, NV) atomic_store_int_nv((P), (NV), __ATOMIC_SEQ_CST)
#define atomic_store_long(P, NV) atomic_store_long_nv((P), (NV), __ATOMIC_SEQ_CST)

#endif  /* !_SYS_ATOMIC_H_ */

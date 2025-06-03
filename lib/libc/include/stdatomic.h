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

#ifndef _STDATOMIC_H
#define _STDATOMIC_H 1

#include <stddef.h>
#include <stdint.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDATOMIC_H__ 202311L
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#define kill_dependency(y) (y)

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L) || defined(__cplusplus)
/* Deprecated in C17, removed in C23 */
#define ATOMIC_VAR_INIT(value) (value)
#endif

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) || defined(__cplusplus)
#define ATOMIC_FLAG_INIT { false }
typedef _Atomic(bool) atomic_bool;
#else
#define ATOMIC_FLAG_INIT { 0 }
typedef _Atomic(_Bool) atomic_bool;
#endif

typedef _Atomic(signed char)        atomic_schar;

typedef _Atomic(char)               atomic_char;
typedef _Atomic(short)              atomic_short;
typedef _Atomic(int)                atomic_int;
typedef _Atomic(long)               atomic_long;
typedef _Atomic(long long)          atomic_llong;

typedef _Atomic(unsigned char)      atomic_uchar;
typedef _Atomic(unsigned short)     atomic_ushort;
typedef _Atomic(unsigned int)       atomic_uint;
typedef _Atomic(unsigned long)      atomic_ulong;
typedef _Atomic(unsigned long long) atomic_ullong;

typedef _Atomic(uintptr_t)          atomic_uintptr_t;
typedef _Atomic(size_t)             atomic_size_t;

typedef struct atomic_flag {
    atomic_bool _Value;
} atomic_flag;

typedef enum memory_order {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

#define atomic_is_lock_free(obj)          __atomic_is_lock_free(sizeof(*(obj)), 0)
#define atomic_flag_test_and_set(obj)     __atomic_test_and_set((obj), memory_order_seq_cst)
#define atomic_flag_clear(obj)            __atomic_clear((obj), memory_order_seq_cst)
#define atomic_load(obj)                  __atomic_load_n((obj), memory_order_seq_cst)
#define atomic_store(obj, desired)        __atomic_store_n((obj), (desired), memory_order_seq_cst)
#define atomic_exchange(obj, desired)     __atomic_exchange_n((obj), (desired), memory_order_seq_cst)
#define atomic_fetch_add(obj, arg)        __atomic_fetch_add((obj), (arg), memory_order_seq_cst)
#define atomic_fetch_sub(obj, arg)        __atomic_fetch_sub((obj), (arg), memory_order_seq_cst)
#define atomic_fetch_and(obj, arg)        __atomic_fetch_and((obj), (arg), memory_order_seq_cst)
#define atomic_fetch_or(obj, arg)         __atomic_fetch_or((obj), (arg), memory_order_seq_cst)
#define atomic_fetch_xor(obj, arg)        __atomic_fetch_xor((obj), (arg), memory_order_seq_cst)

#define atomic_signal_fence               __atomic_signal_fence
#define atomic_thread_fence               __atomic_thread_fence
#define atomic_flag_test_and_set_explicit __atomic_test_and_set
#define atomic_flag_clear_explicit        __atomic_clear
#define atomic_load_explicit              __atomic_load_n
#define atomic_store_explicit             __atomic_store_n
#define atomic_exchange_explicit          __atomic_exchange_n
#define atomic_fetch_add_explicit         __atomic_fetch_add
#define atomic_fetch_sub_explicit         __atomic_fetch_sub
#define atomic_fetch_and_explicit         __atomic_fetch_and
#define atomic_fetch_or_explicit          __atomic_fetch_or
#define atomic_fetch_xor_explicit         __atomic_fetch_xor

#define atomic_compare_exchange_strong(obj, expected, desired) __atomic_compare_exchange_n((obj), (expected), (desired), false, memory_order_seq_cst, memory_order_seq_cst)
#define atomic_compare_exchange_weak(obj, expected, desired)   __atomic_compare_exchange_n((obj), (expected), (desired), true, memory_order_seq_cst, memory_order_seq_cst)
#define atomic_compare_exchange_strong_explicit __atomic_compare_exchange_n
#define atomic_compare_exchange_weak_explicit   __atomic_compare_exchange_n

#endif

#endif /* !_STDATOMIC_H */

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

/* $Id$ */

#ifndef _SYS_CDEFS_H_
#define _SYS_CDEFS_H_

#if !defined(__ASSEMBLER__)

/* Misc helpers */
#define __attr(x)   __attribute__((x))
#define __used      __attr(used)
#define __weak      __attr(weak)
#define __used      __attr(used)

/* Pack a structure */
#define __packed        __attribute__((__packed__))

/* Align by `n` */
#define __aligned(n)    __attribute__((__aligned__(n)))

/*
 * Align to a cacheline-boundary which is
 * typically 64 bytes.
 *
 * XXX: Should probably deal with the case of the
 *      cacheline alignment boundary not being 64 bytes.
 */
#define __cacheline_aligned __aligned(64)

/*
 * To be used when an empty body is required like:
 *
 * #ifdef DEBUG
 * #define dprintf(a) printf(a)
 * #else
 * #define dprintf(a) __nothing
 * #endif
 */
#define __nothing   ((void)0)

/* __BIT(n): Set nth bit, where __BIT(0) == 0x1 */
#define __BIT(n)    (1ULL << n)

/* MASK(n): Sets first n bits, where __MASK(2) == 0b11 */
#define __MASK(n)    (__BIT(n) - 1)

/* Max/min helpers */
#define __MIN(a, b) ((a <= b) ? (a) : (b))
#define __MAX(a, b) ((a >= b) ? (a) : (b))

/* Aligns up/down a value */
#define __ALIGN_DOWN(value, align)      ((value) & ~((align)-1))
#define __ALIGN_UP(value, align)        (((value) + (align)-1) & ~((align)-1))

/* Rounds up and divides */
#define __DIV_ROUNDUP(value, div) __extension__ ({ \
    __auto_type __val = value;                     \
    __auto_type __div = div;                       \
    (__val + (__div - 1)) / __div;                 \
})

/* Find least significant bit that is set */
#define __LOWEST_SET_BIT(mask) ((((mask) - 1) & (mask)) ^ (mask))

/* Extract value with `mask` from `x` */
#define __SHIFTOUT(x, mask) (((x) & (mask)) / __LOWEST_SET_BIT(mask))

/* Test if bits are set, where __TEST(0b1111, 0xF) == 1 */
#define __TEST(a, mask) (__SHIFTOUT(a, mask) != 0)

/* Return the number of elements within an array */
#define __ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

/* Suppress `variable set but not used' warnings */
#define __USE(x) ((void)(x))

/* Computes 2^x i.e 2 to the power of 'x' */
#define __POW2(x) (1ULL << x)

/* Wrapper for inline asm */
#define __ASMV __asm__ __volatile__

/*
 * Used to give metadata to
 * a specific module. Example
 * metadata string:
 *
 * $Vega$: module.c, Programmer Bob, A module that does stuff and things
 * ~~~~~~  ~~~~~~~~  ~~~~~~~~~~~~~~  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Cookie; Module     Author of this        A short description
 * always  name           module
 * first
 *
 * Example usage:
 *
 * __KERNEL_META("$Vega$: module.c, Programmer Bob, "
 *               "A module that does stuff and things");
 *
 * The above is the preferred style for this
 * macro.
 */
#define __KERNEL_META(meta_str)                         \
            __asm__(".section .meta.note\n"             \
                   ".align 4\n"                         \
                   ".string \"" meta_str "\"\n"         \
                   ".previous"                          \
            )

#define __MODULE_NAME(name) \
    __used static const char *__THIS_MODULE = name

#endif  /* !defined(__ASSEMBLER__) */
#endif  /* !_SYS_CDEFS_H_ */

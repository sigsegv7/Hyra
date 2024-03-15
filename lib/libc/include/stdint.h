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

#ifndef _STDINT_H
#define _STDINT_H

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__GNUC__) && \
  ( (__GNUC__ >= 4) || \
    ( (__GNUC__ >= 3) && defined(__GNUC_MINOR__) && (__GNUC_MINOR__ > 2) ) )
#define __STDINT_EXP(x) __##x##__
#else
#define __STDINT_EXP(x) x
#include <limits.h>
#endif

#if __STDINT_EXP(SCHAR_MAX) == 0x7f
typedef signed char int8_t ;
typedef unsigned char uint8_t ;
#define __int8_t_defined 1
#endif

#if __STDINT_EXP(SHRT_MAX) == 0x7fff
typedef signed short int16_t;
typedef unsigned short uint16_t;
#define __int16_t_defined 1
#elif __STDINT_EXP(INT_MAX) == 0x7fff
typedef signed int int16_t;
typedef unsigned int uint16_t;
#define __int16_t_defined 1
#elif __STDINT_EXP(SCHAR_MAX) == 0x7fff
typedef signed char int16_t;
typedef unsigned char uint16_t;
#define __int16_t_defined 1
#endif

#if __have_long32
typedef signed long int32_t;
typedef unsigned long uint32_t;
#define __int32_t_defined 1
#elif __STDINT_EXP(INT_MAX) == 0x7fffffffL
typedef signed int int32_t;
typedef unsigned int uint32_t;
#define __int32_t_defined 1
#elif __STDINT_EXP(SHRT_MAX) == 0x7fffffffL
typedef signed short int32_t;
typedef unsigned short uint32_t;
#define __int32_t_defined 1
#elif __STDINT_EXP(SCHAR_MAX) == 0x7fffffffL
typedef signed char int32_t;
typedef unsigned char uint32_t;
#define __int32_t_defined 1
#endif

#if __SIZEOF_SIZE_T__ == 8
typedef unsigned long long uint64_t;
#endif

#if defined(__PTRDIFF_TYPE__)
typedef signed __PTRDIFF_TYPE__ intptr_t;
typedef unsigned __PTRDIFF_TYPE__ uintptr_t;
#define INTPTR_MAX PTRDIFF_MAX
#define INTPTR_MIN PTRDIFF_MIN
#ifdef __UINTPTR_MAX__
#define UINTPTR_MAX __UINTPTR_MAX__
#else
#define UINTPTR_MAX (2UL * PTRDIFF_MAX + 1)
#endif
#else
typedef signed long intptr_t;
typedef unsigned long uintptr_t;
#define INTPTR_MAX __STDINT_EXP(LONG_MAX)
#define INTPTR_MIN (-__STDINT_EXP(LONG_MAX) - 1)
#define UINTPTR_MAX (__STDINT_EXP(LONG_MAX) * 2UL + 1)
#endif

#if defined(__cplusplus)
}
#endif

#endif  /* !_STDINT_H */

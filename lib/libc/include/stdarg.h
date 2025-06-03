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

#ifndef _STDARG_H
#define _STDARG_H 1

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDARG_H__ 202311L
#endif

/* Determine which definitions are needed */
#if !defined(__need___va_list) && !defined(__need_va_list) && \
    !defined(__need_va_arg) &&                                \
    !defined(__need___va_copy) && !defined(__need_va_copy)
#define __need___va_list
#define __need_va_list
#define __need_va_arg
#define __need___va_copy
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || (defined(__cplusplus) && __cplusplus >= 201103L)
#define __need_va_copy
#endif
#endif

/* __gnuc_va_list type */
#ifdef __need___va_list
#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
typedef __builtin_va_list __gnuc_va_list;
#endif /* !__GNUC_VA_LIST */
#undef __need___va_list
#endif /* __need___va_list */

/* va_list type */
#ifdef __need_va_list
#ifndef _VA_LIST
#define _VA_LIST
typedef __builtin_va_list va_list;
#endif /* !_VA_LIST */
#undef __need_va_list
#endif /* __need_va_list */

/* va_start(), va_end(), and va_arg() macros */
#ifdef __need_va_arg
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define va_start(ap, ...)    __builtin_va_start(ap, 0)
#else
#define va_start(ap, arg)    __builtin_va_start(ap, arg)
#endif
#define va_end(ap)           __builtin_va_end(ap)
#define va_arg(ap, type)     __builtin_va_arg(ap, type)
#undef __need_va_arg
#endif /* __need_va_arg */

/* __va_copy() macro */
#ifdef __need___va_copy
#define __va_copy(dest, src) __builtin_va_copy(dest, src)
#undef __need___va_copy
#endif /* __need___va_copy */

/* va_copy() macro */
#ifdef __need_va_copy
#define va_copy(dest, src)   __builtin_va_copy(dest, src)
#undef __need_va_copy
#endif /* __need_va_copy */

#endif /* !_STDARG_H */

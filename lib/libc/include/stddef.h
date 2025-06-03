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

#ifndef _STDDEF_H
#define _STDDEF_H 1

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDDEF_H__ 202311L
#endif

/* Determine which definitions are needed */
#if !defined(__need_NULL) && !defined(__need_nullptr_t) &&        \
    !defined(__need_size_t) && !defined(__need_rsize_t) &&        \
    !defined(__need_wchar_t) && !defined(__need_wint_t) &&        \
    !defined(__need_ptrdiff_t) && !defined(__need_max_align_t) && \
    !defined(__need_offsetof) && !defined(__need_unreachable)
#define __need_NULL
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) || defined(__cplusplus)
#define __need_nullptr_t
#endif
#define __need_ptrdiff_t
#define __need_size_t
#if defined(__STDC_WANT_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ >= 1
#define __need_rsize_t
#endif
#define __need_wchar_t
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || (defined(__cplusplus) && __cplusplus >= 201103L)
#define __need_max_align_t
#endif
#define __need_offsetof
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __need_unreachable
#endif
#endif

/* NULL pointer constant */
#ifdef __need_NULL
#ifdef __cplusplus
#if __cplusplus >= 201103L
#define NULL nullptr
#else
#define NULL 0L
#endif /* __cplusplus >= 201103L */
#else
#define NULL ((void *) 0)
#endif /* __cplusplus */
#undef __need_NULL
#endif /* __need_NULL */

/* nullptr_t type */
#ifdef __need_nullptr_t
#ifndef _NULLPTR_T
#define _NULLPTR_T
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
typedef typeof(nullptr)   nullptr_t;
#endif
#endif /* !_NULLPTR_T */
#undef __need_nullptr_t
#endif /* __need_nullptr_t */

/* size_t type */
#ifdef __need_size_t
#ifndef _SIZE_T
#define _SIZE_T
#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__     size_t;
#else
typedef long unsigned int size_t;
#endif /* __SIZE_TYPE__ */
#endif /* !_SIZE_T */
#undef __need_size_t
#endif /* __need_size_t */

/* rsize_t type */
#ifdef __need_rsize_t
#ifndef _RSIZE_T
#define _RSIZE_T
#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__     rsize_t;
#else
typedef long unsigned int rsize_t;
#endif /* __SIZE_TYPE__ */
#endif /* !_RSIZE_T */
#undef __need_rsize_t
#endif /* __need_rsize_t */

/* wchar_t type */
#ifdef __need_wchar_t
#ifndef _WCHAR_T
#define _WCHAR_T
#ifdef __WCHAR_TYPE__
typedef __WCHAR_TYPE__    wchar_t;
#else
typedef int               wchar_t;
#endif /* __WCHAR_TYPE__ */
#endif /* !_WCHAR_T */
#undef __need_wchar_t
#endif /* __need_wchar_t */

/* wint_t type */
#ifdef __need_wint_t
#ifndef _WINT_T
#define _WINT_T
#ifdef __WINT_TYPE__
typedef __WINT_TYPE__     wint_t;
#else
typedef unsigned int      wint_t;
#endif /* __WINT_TYPE__ */
#endif /* !_WINT_T */
#undef __need_wint_t
#endif /* __need_wint_t */

/* ptrdiff_t type */
#ifdef __need_ptrdiff_t
#ifndef _PTRDIFF_T
#define _PTRDIFF_T
#ifdef __PTRDIFF_TYPE__
typedef __PTRDIFF_TYPE__  ptrdiff_t;
#else
typedef long int          ptrdiff_t;
#endif /* __PTRDIFF_TYPE__ */
#endif /* !_PTRDIFF_T */
#undef __need_ptrdiff_t
#endif /* __need_ptrdiff_t */

/* max_align_t type */
#ifdef __need_max_align_t
#if defined (_MSC_VER)
typedef double            max_align_t;
#elif defined(__APPLE__)
typedef long double       max_align_t;
#else
typedef struct {
    long long   __longlong   __attribute__((__aligned__(__alignof__(long long))));
    long double __longdouble __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;
#endif
#undef __need_max_align_t
#endif /* __need_max_align_t */

/* offsetof() macro */
#ifdef __need_offsetof
#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif
#undef __need_offsetof
#endif /* __need_offsetof */

/* unreachable() macro */
#ifdef __need_unreachable
/* C++ has std::unreachable() */
#if !defined(__cplusplus) && !defined(unreachable)
#define unreachable()          __builtin_unreachable()
#endif
#undef __need_unreachable
#endif /* __need_unreachable */

#endif  /* !_STDDEF_H */

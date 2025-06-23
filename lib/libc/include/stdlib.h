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

#ifndef _STDLIB_H
#define _STDLIB_H 1

/* For __dead */
#include <sys/cdefs.h>

/* Get specific definitions from stddef.h */
#define __need_NULL
#define __need_size_t
#define __need_wchar_t
#include <stddef.h>

#if __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDLIB_H__ 202311L
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

typedef struct {
    int quot;
    int rem;
} div_t;

#ifndef __ldiv_t_defined
typedef struct {
    long int quot;
    long int rem;
} ldiv_t;
#define __ldiv_t_defined 1
#endif /* !__ldiv_t_defined */

#ifndef __lldiv_t_defined
typedef struct {
    long long int quot;
    long long int rem;
} lldiv_t;
#define __lldiv_t_defined 1
#endif /* !__lldiv_t_defined */

__BEGIN_DECLS

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) || (defined(__cplusplus) && __cplusplus >= 201103L)
[[noreturn]] void abort(void);
[[noreturn]] void exit(int status);
[[noreturn]] void _Exit(int status);
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Noreturn void abort(void);
_Noreturn void exit(int status);
_Noreturn void _Exit(int status);
#else
__dead void abort(void);
__dead void exit(int status);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
__dead void _Exit(int status);
#endif
#endif

void *malloc(size_t size);
void free(void *ptr);

void srand(unsigned int r);
int rand(void);

__END_DECLS

#endif /* !_STDLIB_H */

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

#ifndef _CTYPE_H
#define _CTYPE_H 1

#include <sys/param.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

__always_inline static inline int
__tolower(int c)
{
    return c | 0x20;
}

__always_inline static inline int
__toupper(int c)
{
    return c & ~0x20;
}

__always_inline static inline int
__isalpha(int c)
{
    c = __tolower(c);
    return c >= 'a' && c <= 'z';
}

__always_inline static inline int
__isdigit(int c)
{
    return c >= '0' && c <= '9';
}

__always_inline static inline int
__isspace(int c)
{
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\v':
        return 1;

    return 0;
}

__END_DECLS

/* Conver char to lowercase */
#define tolower(C) __tolower((C))

/* Conver char to uppercase */
#define toupper(C) __toupper((C))

/* Is alphabetical? */
#define isalpha(C) __isalpha((C))

/* Is a digit? */
#define isdigit(C) __isdigit((C))

/* Is a space? */
#define isspace(C) __isspace((C))

#endif  /* _CTYPE_H */

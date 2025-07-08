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

#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#ifndef _HAVE_STYPES
#define _HAVE_STYPES
#endif

/* Compat */
#if defined(_KERNEL)
#include <stdbool.h>
#if !defined(NULL)
#define NULL ((void *)0)
#endif  /* !NULL */
#endif  /* _KERNEL */

/* Fixed width integer types */
typedef long                __ptrdiff_t;
typedef unsigned char       __uint8_t;
typedef unsigned short      __uint16_t;
typedef unsigned int        __uint32_t;
typedef unsigned long long  __uint64_t;

typedef signed char         __int8_t;
typedef short               __int16_t;
typedef int                 __int32_t;
typedef long long           __int64_t;

/* Fixed width integer type limits */
#define __INT8_MIN        (-0x7F - 1)
#define __INT16_MIN       (-0x7FFF - 1)
#define __INT32_MIN       (-0x7FFFFFFF - 1)
#define __INT64_MIN       (-0x7FFFFFFFFFFFFFFFLL - 1)

#define __INT8_MAX        0x7F
#define __INT16_MAX       0x7FFF
#define __INT32_MAX       0x7FFFFFFF
#define __INT64_MAX       0x7FFFFFFFFFFFFFFFLL

#define __UINT8_MAX       0xFF
#define __UINT16_MAX      0xFFFF
#define __UINT32_MAX      0xFFFFFFFFU
#define __UINT64_MAX      0xFFFFFFFFFFFFFFFFULL

typedef __int8_t        int8_t;
typedef __uint8_t       uint8_t;
typedef __int16_t       int16_t;
typedef __uint16_t      uint16_t;
typedef __int32_t       int32_t;
typedef __uint32_t      uint32_t;
#if __SIZEOF_LONG__ == 8
typedef __int64_t       int64_t;
typedef __uint64_t      uint64_t;
#endif

#if __SIZEOF_SIZE_T__ == 8
typedef uint64_t    __size_t;
typedef int64_t     __ssize_t;       /* Byte count or error */
#elif __SIZEOF_SIZE_T__ == 4
typedef uint32_t    __size_t;
typedef int32_t     __ssize_t;       /* Byte count or error */
#else
#error "Unsupported size_t size"
#endif

#if defined(_KERNEL) || defined(_HYRA)
typedef __size_t    size_t;
#endif
typedef __ssize_t   ssize_t;

typedef __size_t uintptr_t;
typedef __size_t off_t;
typedef int pid_t;
typedef int dev_t;
typedef uint32_t uid_t;
typedef uint32_t mode_t;
typedef uint32_t ino_t;
typedef uint32_t nlink_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t blksize_t;
typedef uint32_t blkcnt_t;
typedef uint64_t time_t;
#if defined(_HAVE_PTRDIFF_T)
typedef __ptrdiff_t ptrdiff_t;
#endif  /* _HAVE_PTRDIFF_T */

#if defined(_KERNEL)
typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;
#endif  /* _KERNEL */

#endif  /* _SYS_TYPES_H_ */

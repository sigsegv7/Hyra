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

#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#if !defined(NULL)
# define NULL (void *)0
#endif

typedef signed char     int8_t;
typedef unsigned char   uint8_t;
typedef short           int16_t;
typedef unsigned short  uint16_t;
typedef int             int32_t;
typedef unsigned int    uint32_t;
#if __SIZEOF_LONG__ == 8
typedef long            int64_t;
typedef unsigned long   uint64_t;
#elif __SIZEOF_LONG__ == 4
__extension__
typedef long long       int64_t;
__extension__
typedef unsigned long long uint64_t;
#else
#error "Unsupported long size"
#endif

#if __SIZEOF_SIZE_T__ == 8
typedef uint64_t        size_t;
typedef int64_t  ssize_t;       /* Byte count or error */
#elif __SIZEOF_SIZE_T__ == 4
typedef uint32_t        size_t;
typedef int32_t         ssize_t;        /* Byte count or error */
#else
#error "Unsupported size_t size"
#endif

typedef size_t          uintptr_t;
#ifndef __ptrdiff_defined
typedef size_t          ptrdiff_t;
#endif
typedef size_t          off_t;

typedef size_t          id_t;
typedef id_t            pid_t;
typedef id_t            tid_t;
typedef id_t            uid_t;
typedef id_t            gid_t;
typedef uint32_t        ino_t;
typedef int32_t         dev_t;
typedef int32_t         mode_t;
typedef int32_t         nlink_t;
typedef _Bool           bool;

#endif   /* _SYS_TYPES_H_ */

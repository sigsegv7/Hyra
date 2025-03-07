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

#ifndef _BITS_TYPES_H
#define _BITS_TYPES_H

/* Fixed width integer types */
typedef unsigned long       __size_t;
typedef long                __ssize_t;
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

#endif  /* !_BITS_TYPES_H */

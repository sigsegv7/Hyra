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

#ifndef _SYS_BITOPS_H_
#define _SYS_BITOPS_H_

#include <sys/cdefs.h>

#define BOPS_M1  0x5555555555555555ULL /* 01010101... */
#define BOPS_M2  0x3333333333333333ULL /* 00110011... */
#define BOPS_M4  0x0F0F0F0F0F0F0F0FULL /* 00001111... */
#define BOPS_M8  0x00FF00FF00FF00FFULL /* x4(0), x4(1) */
#define BOPS_M16 0x0000FFFF0000FFFFULL /* x16(0), x16(1) */
#define BOPS_M32 0x00000000FFFFFFFFULL /* x32(0), x32(1) */
#define BOPS_H0  0x0101010101010101ULL /* sum of 256^{0,1,2,3...} */

__always_inline static inline int
popcnt(uint64_t x)
{
    x -= (x >> 1) & BOPS_M1;
    x =  (x & BOPS_M2) + ((x >> 2) & BOPS_M2);
    x =  (x + (x >> 4)) & BOPS_M4;
    return (x * BOPS_H0) >> 56;
}

#endif  /* !_SYS_BITOPS_H_ */

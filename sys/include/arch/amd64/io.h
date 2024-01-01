/*
 * Copyright (c) 2024 Ian Marco Moffett and the Osmora Team.
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

#ifndef _AMD64_IO_H_
#define _AMD64_IO_H_

#include <sys/types.h>
#include <sys/cdefs.h>

static inline uint8_t
inb(uint16_t port)
{
    uint8_t result;
    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

static inline void
outb(uint16_t port, uint8_t data)
{
    __asm__("out %%al, %%dx" : :"a" (data), "d" (port));
}

static inline void
outw(uint16_t port, uint16_t data)
{
    __ASMV("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

static inline uint16_t
inw(uint16_t port)
{
    uint16_t data;
    __ASMV("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void
outl(uint16_t port, uint32_t data)
{
    __ASMV("outl %0, %w1" : : "a" (data), "Nd" (port));
}

static inline uint32_t
inl(uint16_t port)
{
    uint32_t data;
    __ASMV("inl %w1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

#endif  /* !_AMD64_IO_H_ */

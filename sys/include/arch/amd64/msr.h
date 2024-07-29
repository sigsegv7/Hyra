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

#ifndef _MACHINE_MSR_H_
#define _MACHINE_MSR_H_

#define IA32_SPEC_CTL       0x00000048
#define IA32_KERNEL_GS_BASE 0xC0000102
#define IA32_FS_BASE        0xC0000100
#define IA32_APIC_BASE_MSR  0x0000001B

#if !defined(__ASSEMBLER__)
static inline uint64_t
rdmsr(uint32_t msr_addr)
{
    uint32_t lo, hi;

    __ASMV("rdmsr"
           : "=a" (lo), "=d" (hi)
           : "c" (msr_addr)
    );
    return ((uint64_t)hi << 32) | lo;
}

static inline void
wrmsr(uint32_t msr_addr, uint64_t value)
{
    uint32_t lo, hi;

    lo = (uint32_t)value;
    hi = (uint32_t)(value >> 32);

    __ASMV("wrmsr"
           :  /* No outputs */
           : "a" (lo), "d" (hi),
             "c" (msr_addr)
    );
}

#endif  /* !__ASSEMBLER__ */
#endif  /* !_MACHINE_MSR_H_ */

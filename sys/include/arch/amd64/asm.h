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

#ifndef _MACHINE_ASM_H_
#define _MACHINE_ASM_H_

#include <sys/types.h>
#include <sys/param.h>
#include <machine/msr.h>

/*
 * Contains information for the current
 * core. Stored in %GS.
 *
 * MUST REMAIN IN ORDER!!!
 */
struct cpu_ctx {
    struct cpu_info *ci;
};

/*
 * Returns true for this core if maskable
 * interrupts are masked (CLI) and false if
 * they aren't (STI).
 */
static inline bool
amd64_is_intr_mask(void)
{
    uint64_t flags;

    __ASMV("pushfq; pop %0" : "=rm" (flags) :: "memory");
    return !ISSET(flags, BIT(9));
}

static inline void
amd64_write_gs_base(uintptr_t val)
{
    wrmsr(IA32_KERNEL_GS_BASE, val);
}

static inline uintptr_t
amd64_read_gs_base(void)
{
    return rdmsr(IA32_KERNEL_GS_BASE);
}

static inline uint64_t
amd64_read_cr0(void)
{
    uint64_t cr0;
    __ASMV("mov %%cr0, %0" : "=r" (cr0) :: "memory");
    return cr0;
}

static inline void
amd64_write_cr0(uint64_t val)
{
    __ASMV("mov %0, %%cr0" :: "r" (val) : "memory");
}

static inline uint64_t
amd64_read_cr4(void)
{
    uint64_t cr4;
    __ASMV("mov %%cr4, %0" : "=r" (cr4) :: "memory");
    return cr4;
}

static inline void
amd64_write_cr4(uint64_t val)
{
    __ASMV("mov %0, %%cr4" :: "r" (val) : "memory");
}

static inline void
amd64_fxsave(void *area)
{
    __ASMV("fxsave (%0)" :: "r" (area) : "memory");
}

static inline void
amd64_fxrstor(void *area)
{
    __ASMV("fxrstor (%0)" :: "r" (area) : "memory");
}

#endif

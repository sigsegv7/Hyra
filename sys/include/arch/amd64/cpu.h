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

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/spinlock.h>
#include <machine/tss.h>
#include <machine/cdefs.h>
#include <machine/intr.h>

#define CPU_IRQ(IRQ_N) (BIT((IRQ_N)) & 0xFF)

/* Feature bits */
#define CPU_FEAT_SMAP  BIT(0)
#define CPU_FEAT_SMEP  BIT(1)

/* CPU vendors */
#define CPU_VENDOR_OTHER    0x00000000
#define CPU_VENDOR_INTEL    0x00000001
#define CPU_VENDOR_AMD      0x00000002

typedef uint16_t ipi_pend_t;

struct cpu_info {
    uint32_t apicid;
    uint32_t feat;
    uint32_t vendor;            /* Vendor (see CPU_VENDOR_*) */
    uint8_t ipi_dispatch : 1;   /* 1: IPIs being dispatched */
    uint8_t ipi_id;
    ipi_pend_t ipi_pending[N_IPIVEC];
    uint8_t id;                 /* MI Logical ID */
    uint8_t model : 4;          /* CPU model number */
    uint8_t family : 4;         /* CPU family ID */
    uint8_t has_x2apic : 1;
    uint8_t tlb_shootdown : 1;
    uint8_t online : 1;         /* CPU online */
    uint8_t ipl;
    size_t lapic_tmr_freq;
    uint8_t irq_mask;
    vaddr_t shootdown_va;
    struct sched_cpu stat;
    struct tss_entry *tss;
    struct proc *curtd;
    struct spinlock lock;
    struct cpu_info *self;
};

__dead void cpu_halt_all(void);
void cpu_halt_others(void);
void cpu_startup(struct cpu_info *ci);

void cpu_enable_smep(void);
void cpu_disable_smep(void);

struct cpu_info *cpu_get(uint32_t index);
struct sched_cpu *cpu_get_stat(uint32_t cpu_index);

uint32_t cpu_count(void);
void cpu_shootdown_tlb(vaddr_t va);

struct cpu_info *this_cpu(void);
void mp_bootstrap_aps(struct cpu_info *ci);

extern struct cpu_info g_bsp_ci;

__always_inline static inline void
cpu_halt(void)
{
    struct cpu_info *ci = this_cpu();

    if (ci != NULL)
        ci->online = 0;

    __ASMV("hlt");

    if (ci != NULL)
        ci->online = 1;
}

#endif  /* !_MACHINE_CPU_H_ */

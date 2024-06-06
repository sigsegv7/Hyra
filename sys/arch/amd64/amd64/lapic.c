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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/panic.h>
#include <sys/mmio.h>
#include <sys/syslog.h>
#include <machine/lapicvar.h>
#include <machine/lapic.h>
#include <machine/cpuid.h>
#include <machine/cpu.h>
#include <machine/msr.h>

#define pr_trace(fmt, ...) kprintf("lapic: " fmt, ##__VA_ARGS__)

/*
 * Only calls pr_trace if we are the BSP.
 */
#define bsp_trace(...) do {                     \
        uint64_t msr_val;                       \
                                                \
        msr_val = rdmsr(IA32_APIC_BASE_MSR);    \
        if (ISSET(msr_val, BIT(8))) {           \
            pr_trace(__VA_ARGS__);              \
        }                                       \
    } while (0);

uintptr_t g_lapic_base = 0;

/*
 * Returns true if LAPIC is supported.
 *
 * LAPIC is supported if CPUID.(EAX=1H):EDX[9] == 1
 */
static inline bool
lapic_supported(void)
{
    uint32_t eax, edx, tmp;

    CPUID(0x00000001, eax, tmp, tmp, edx);
    return ISSET(edx, BIT(9));
}

/*
 * Checks if the processor supports x2APIC
 * mode. Returns true if so.
 */
static inline bool
lapic_has_x2apic(void)
{
    uint32_t ecx, tmp;

    CPUID(0x00000001, tmp, tmp, ecx, tmp);
    return ISSET(ecx, BIT(21));
}

/*
 * Reads a 32 bit value from Local APIC
 * register space.
 *
 * @reg: Register to read from.
 */
static inline uint64_t
lapic_readl(uint32_t reg)
{
    void *addr;
    const struct cpu_info *ci = this_cpu();

    if (!ci->has_x2apic) {
        addr = (void *)(g_lapic_base + reg);
        return mmio_read32(addr);
    } else {
        reg >>= 4;
        return rdmsr(x2APIC_MSR_BASE + reg);
    }
}

/*
 * Writes a 32 bit value to Local APIC
 * register space.
 *
 * @reg: Register to write to.
 */
static inline void
lapic_writel(uint32_t reg, uint64_t val)
{
    void *addr;
    const struct cpu_info *ci = this_cpu();

    if (!ci->has_x2apic) {
        addr = (void *)(g_lapic_base + reg);
        mmio_write32(addr, val);
    } else {
        reg >>= 4;
        wrmsr(x2APIC_MSR_BASE + reg, val);
    }
}

/*
 * Set bits within a LAPIC register
 * without overwriting the whole thing.
 *
 * @reg: Reg with bits to be set.
 * @value: Value in reg will be ORd with this.
 */
static inline void
lapic_reg_set(uint32_t reg, uint32_t value)
{
    uint32_t old;

    old = lapic_readl(reg);
    lapic_writel(reg, old | value);
}

/*
 * Clear bits within a LAPIC register
 * without overwriting the whole thing.
 *
 * @reg: Reg with bits to be cleared.
 * @value: Value in reg will be cleared by this value.
 */
static inline void
lapic_reg_clear(uint32_t reg, uint32_t value)
{
    uint32_t old;

    old = lapic_readl(reg);
    lapic_writel(reg, old & ~(value));
}

/*
 * Hardware and software enable the Local APIC
 * through IA32_APIC_BASE_MSR
 */
static inline void
lapic_enable(const struct cpu_info *ci)
{
    uint64_t tmp;

    /* Hardware enable the Local APIC */
    tmp = rdmsr(IA32_APIC_BASE_MSR);
    tmp |= ci->has_x2apic << x2APIC_ENABLE_SHIFT;
    wrmsr(IA32_APIC_BASE_MSR, tmp | LAPIC_HW_ENABLE);

    /* Software enable the Local APIC */
    lapic_reg_set(LAPIC_SVR, LAPIC_SW_ENABLE);
}

/*
 * Reads the Local APIC ID of the current
 * processor.
 */
static inline uint32_t
lapic_read_id(const struct cpu_info *ci)
{
    if (!ci->has_x2apic) {
        return (lapic_readl(LAPIC_ID) >> 24) & 0xF;
    } else {
        return lapic_readl(LAPIC_ID);
    }
}

void
lapic_init(void)
{
    struct cpu_info *ci = this_cpu();

    /*
     * Hyra currently depends on the existance
     * of a Local APIC.
     */
    if (!lapic_supported()) {
        panic("This machine does not support LAPIC!\n");
    }

    /* Ensure the LAPIC base is valid */
    if (g_lapic_base == 0) {
        panic("Invalid LAPIC base\n");
    }

    ci->has_x2apic = lapic_has_x2apic();
    lapic_enable(ci);

    ci->apicid = lapic_read_id(ci);
    bsp_trace("BSP LAPIC enabled in %s mode (id=%d)\n",
        ci->has_x2apic ? "x2APIC" : "xAPIC", ci->apicid);
}

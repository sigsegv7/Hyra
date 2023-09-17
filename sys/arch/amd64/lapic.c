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

#include <machine/lapic.h>
#include <machine/lapicvar.h>
#include <machine/cpuid.h>
#include <machine/msr.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/mmio.h>

/*
 * Only calls KINFO if we are the BSP.
 */
#define BSP_KINFO(...)                      \
    uint64_t msr_val;                       \
                                            \
    msr_val = rdmsr(IA32_APIC_BASE_MSR);    \
    if (__TEST(msr_val, 1 << 8)) {          \
        KINFO(__VA_ARGS__);                 \
    }

__MODULE_NAME("lapic");
__KERNEL_META("$Vega$: lapic.c, Ian Marco Moffett, "
              "Local APIC driver");

static void *lapic_base = NULL;

/*
 * Returns true if LAPIC is supported.
 *
 * LAPIC is supported if CPUID.(EAX=1H):EDX[9] == 1
 */
static inline bool
lapic_check_support(void)
{
    uint32_t eax, edx, tmp;

    __CPUID(0x00000001, eax, tmp, tmp, edx);
    return __TEST(edx, 1 << 9);
}

/*
 * Reads a 32 bit value from Local APIC
 * register space.
 *
 * @reg: Register to read from.
 */
static inline uint32_t
lapic_readl(uint32_t reg)
{
    void *addr;

    addr = (void *)((uintptr_t)lapic_base + reg);
    return mmio_read32(addr);
}

/*
 * Writes a 32 bit value to Local APIC
 * register space.
 *
 * @reg: Register to write to.
 */
static inline void
lapic_writel(uint32_t reg, uint32_t val)
{
    void *addr;

    addr = (void *)((uintptr_t)lapic_base + reg);
    mmio_write32(addr, val);
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

void
lapic_set_base(void *mmio_base)
{
    if (lapic_base == NULL)
        lapic_base = mmio_base;
}

void
lapic_init(void)
{
    uint64_t tmp;

    /* Sanity check */
    if (lapic_base == NULL) {
        panic("LAPIC base not set!\n");
    }

    if (!lapic_check_support()) {
        /*
         * VegaOS currently depends on the existance
         * of a Local APIC.
         */
        panic("This machine does not support LAPIC!\n");
    }

    /* Hardware enable the Local APIC */
    tmp = rdmsr(IA32_APIC_BASE_MSR);
    wrmsr(IA32_APIC_BASE_MSR, tmp | LAPIC_HW_ENABLE);

    /* Software enable the Local APIC via SVR */
    lapic_reg_set(LAPIC_SVR, LAPIC_SW_ENABLE);

    BSP_KINFO("Enabled Local APIC for BSP\n");
    lapic_writel(LAPIC_LDR, LAPIC_STARTUP_LID);
}

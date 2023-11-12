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
#include <machine/cpu.h>
#include <sys/cdefs.h>
#include <sys/timer.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/mmio.h>
#include <dev/timer/hpet.h>

__MODULE_NAME("lapic");
__KERNEL_META("$Vega$: lapic.c, Ian Marco Moffett, "
              "Local APIC driver");

/*
 * Only calls KINFO if we are the BSP.
 */
#define BSP_KINFO(...) do {                     \
        uint64_t msr_val;                       \
                                                \
        msr_val = rdmsr(IA32_APIC_BASE_MSR);    \
        if (__TEST(msr_val, 1 << 8)) {          \
            KINFO(__VA_ARGS__);                 \
        }                                       \
    } while (0);

static void *lapic_base = NULL;
static struct timer lapic_timer = { 0 };
static bool has_x2apic = false;

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

    if (!has_x2apic) {
        addr = (void *)((uintptr_t)lapic_base + reg);
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
lapic_writel(uint32_t reg, uint32_t val)
{
    void *addr;

    if (!has_x2apic) {
        addr = (void *)((uintptr_t)lapic_base + reg);
        mmio_write32(addr, val);
    } else {
        reg >>= 4;
        wrmsr(x2APIC_MSR_BASE + reg, val);
    }
}

/*
 * Calibrates the Local APIC timer - Timer abstraction
 */
static size_t
lapic_timer_calibrate(void)
{
    size_t freq_hz;

    lapic_timer_init(&freq_hz);
    return freq_hz;
}

/*
 * Stops the Local APIC timer - Timer abstraction
 */
static void
lapic_timer_stop(void)
{
    lapic_writel(LAPIC_INIT_CNT, 0);
    lapic_writel(LAPIC_LVT_TMR, LAPIC_LVT_MASK);
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
 * Reads the Local APIC ID of the
 * current processor.
 */
static inline uint32_t
lapic_get_id(void)
{
    if (!has_x2apic) {
        return (lapic_readl(LAPIC_ID) >> 24) & 0xF;
    } else {
        return lapic_readl(LAPIC_ID);
    }
}

/*
 * Checks if the processor supports x2APIC
 * mode. Returns true if so.
 */
static inline bool
lapic_has_x2apic(void)
{
    uint32_t ecx, tmp;

    __CPUID(0x00000001, tmp, tmp, ecx, tmp);
    return __TEST(ecx, 1 << 21);
}

/*
 * Updates LDR to LAPIC_STARTUP_LID.
 *
 * XXX: This does *not* happen with x2APIC
 *      as the LDR register in x2APIC mode
 *      is readonly.
 */
static inline void
lapic_set_ldr(void)
{
    if (!has_x2apic)
        lapic_writel(LAPIC_LDR, LAPIC_STARTUP_LID);
}

void
lapic_timer_init(size_t *freq_out)
{
    uint32_t ticks_per_10ms;
    size_t freq_hz;
    const uint32_t MHZ_DIV = 1000000;
    const uint32_t GHZ_DIV = 1000000000;
    const uint32_t MAX_SAMPLES = 0xFFFFFFFF;

    lapic_writel(LAPIC_DCR, 3);                     /* Use divider 16 */
    lapic_writel(LAPIC_INIT_CNT, MAX_SAMPLES);      /* Set the initial count */

    hpet_msleep(10);                                /* Wait 10ms (10000 usec) */
    lapic_writel(LAPIC_LVT_TMR, LAPIC_LVT_MASK);    /* Stop the timer w/ LVT mask bit */

    /* Sanity check */
    if (freq_out == NULL)
        panic("lapic_timer_init() freq_out NULL\n");

    /* Calculate frequency in Hz */
    ticks_per_10ms = MAX_SAMPLES - lapic_readl(LAPIC_CUR_CNT);
    freq_hz = ticks_per_10ms * 10000;       /* 10000 since we waited 10ms */

    /* Check if in MHz or GHz range */
    if (freq_hz >= GHZ_DIV) {
        BSP_KINFO("BSP Local APIC Timer frequency: %d GHz\n", freq_hz / GHZ_DIV);
    } else if (freq_hz >= MHZ_DIV) {
        BSP_KINFO("BSP Local APIC Timer frequency: %d MHz\n", freq_hz / MHZ_DIV);
    } else {
        BSP_KINFO("BSP Local APIC Timer frequency: %d Hz\n", freq_hz);
    }

    /* Divide by 10000 to get ticks */
    *freq_out = freq_hz;
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
    struct cpu_info *cur_cpu;
    uint64_t tmp;
    size_t tmr_freq;

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

    has_x2apic = lapic_has_x2apic();

    /* Hardware enable the Local APIC */
    tmp = rdmsr(IA32_APIC_BASE_MSR);
    tmp |= has_x2apic << x2APIC_ENABLE_SHIFT;
    wrmsr(IA32_APIC_BASE_MSR, tmp | LAPIC_HW_ENABLE);

    /* Software enable the Local APIC via SVR */
    lapic_reg_set(LAPIC_SVR, LAPIC_SW_ENABLE);

    BSP_KINFO("Enabled Local APIC for BSP\n");
    lapic_set_ldr();

    /* Setup the timer descriptor */
    lapic_timer.name = "LAPIC_INTEGRATED_TIMER";
    lapic_timer.calibrate = lapic_timer_calibrate;
    lapic_timer.stop = lapic_timer_stop;

    /* Register the timer for scheduler usage */
    register_timer(TIMER_SCHED, &lapic_timer);

    /* Get the current processor, and lock its structure */
    cur_cpu = this_cpu();
    CPU_INFO_LOCK(cur_cpu);

    /* Calibrate timer */
    lapic_timer_init(&tmr_freq);
    cur_cpu->lapic_tmr_freq = tmr_freq;

    /* Set the Local APIC ID */
    cur_cpu->lapic_id = lapic_get_id();

    BSP_KINFO("BSP Local APIC ID: %d\n", cur_cpu->lapic_id);
    CPU_INFO_UNLOCK(cur_cpu);
}

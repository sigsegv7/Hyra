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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/panic.h>
#include <sys/mmio.h>
#include <sys/syslog.h>
#include <sys/spinlock.h>
#include <dev/timer.h>
#include <machine/intr.h>
#include <machine/isa/i8254.h>
#include <machine/lapicvar.h>
#include <machine/lapic.h>
#include <machine/cpuid.h>
#include <machine/cpu.h>
#include <machine/msr.h>
#include <machine/idt.h>
#include <machine/tss.h>

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

static struct timer lapic_timer;
static uint8_t lapic_timer_vec = 0;
uintptr_t g_lapic_base = 0;

void lapic_tmr_isr(void);

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
 * Starts the Local APIC countdown timer...
 *
 * @mask: True to mask timer.
 * @mode: Timer mode.
 * @count: Count to start at.
 */
static inline void
lapic_timer_start(bool mask, uint8_t mode, uint32_t count)
{
    uint32_t tmp;

    tmp = (mode << 17) | (mask << 16) | lapic_timer_vec;
    lapic_writel(LAPIC_LVT_TMR, tmp);
    lapic_writel(LAPIC_DCR, 0);
    lapic_writel(LAPIC_INIT_CNT, count);
}

/*
 * Start Local APIC timer oneshot with number
 * of ticks to count down from.
 *
 * @mask: If `true', timer will be masked, `count' should be 0.
 * @count: Number of ticks.
 */
static void
lapic_timer_oneshot(bool mask, uint32_t count)
{
    lapic_timer_start(mask, LVT_TMR_ONESHOT, count);
}

/*
 * Start Local APIC timer oneshot in microseconds.
 *
 * @us: Microseconds.
 */
static void
lapic_timer_oneshot_us(size_t usec)
{
    uint64_t ticks;
    struct cpu_info *ci = this_cpu();

    ticks = usec * (ci->lapic_tmr_freq / 1000000);
    lapic_timer_oneshot(false, ticks);
}

/*
 * Stops the Local APIC timer
 */
static void
lapic_timer_stop(void)
{
    lapic_writel(LAPIC_LVT_TMR, LAPIC_LVT_MASK);
    lapic_writel(LAPIC_INIT_CNT, 0);
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
 * Hardware and software enable the Local APIC
 * through IA32_APIC_BASE_MSR and the SVR.
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

/*
 * Init the Local APIC timer and return
 * the frequency.
 */
static size_t
lapic_timer_init(void)
{
    uint16_t ticks_start, ticks_end;
    size_t ticks_total, freq;
    const uint16_t MAX_SAMPLES = 0xFFFF;
    static struct spinlock lock = {0};

    spinlock_acquire(&lock);

    lapic_timer_stop();
    i8254_set_reload(MAX_SAMPLES);
    ticks_start = i8254_get_count();

    lapic_writel(LAPIC_INIT_CNT, MAX_SAMPLES);
    while (lapic_readl(LAPIC_CUR_CNT) != 0);

    ticks_end = i8254_get_count();
    ticks_total = ticks_start - ticks_end;

    freq = (MAX_SAMPLES / ticks_total) * I8254_DIVIDEND;
    lapic_timer_stop();

    spinlock_release(&lock);
    return freq;
}

void
lapic_send_ipi(uint8_t id, uint8_t shorthand, uint8_t vector)
{
    const uint32_t X2APIC_IPI_SELF = 0x3F0;
    struct cpu_info *ci = this_cpu();
    uint64_t icr_lo = 0;

    /*
     * If we are in x2APIC mode and the shorthand is "self", use
     * the x2APIC SELF IPI register as it is more optimized.
     */
    if (shorthand == IPI_SHORTHAND_SELF && ci->has_x2apic) {
        lapic_writel(X2APIC_IPI_SELF, vector);
        return;
    }

    /* Encode dest into the low dword of the ICR */
    icr_lo |= (vector | IPI_DEST_PHYSICAL);
    icr_lo |= ((shorthand & 0x3) << 18);

    /*
     * In xAPIC mode, the Delivery Status bit (bit 12) must
     * be polled until clear after sending an IPI. However,
     * in x2APIC mode, this bit does not exist, so there's no
     * need to worry about polling. Since the x2APIC interface
     * uses MSRs, we can accomplish what we need with a single
     * write, unlike with xAPICs where you'd need to write to the
     * ICR high dword first.
     */
    if (ci->has_x2apic) {
        lapic_writel(LAPIC_ICRLO, ((uint64_t)id << 32) | icr_lo);
    } else {
        lapic_writel(LAPIC_ICRHI, ((uint32_t)id << 24));
        lapic_writel(LAPIC_ICRLO, icr_lo);
        while (ISSET(lapic_readl(LAPIC_ICRLO), BIT(12)));
    }
}

/*
 * Indicates that the current interrupt is finished
 * being serviced.
 */
void
lapic_eoi(void)
{
    lapic_writel(LAPIC_EOI, 0);
}

void
lapic_init(void)
{
    struct cpu_info *ci = this_cpu();
    union tss_stack tmr_stack;

    /*
     * Hyra currently depends on the existance
     * of a Local APIC.
     */
    if (!lapic_supported()) {
        panic("This machine does not support LAPIC!\n");
    }

    /* Try to allocate LAPIC timer interrupt stack */
    if (tss_alloc_stack(&tmr_stack, DEFAULT_PAGESIZE) != 0) {
        panic("Failed to allocate LAPIC TMR stack!\n");
    }

    tss_update_ist(ci, tmr_stack, IST_SCHED);

    /* Allocate a vector if needed */
    if (lapic_timer_vec == 0) {
        lapic_timer_vec = intr_alloc_vector("lapictmr", IPL_CLOCK);
        idt_set_desc(lapic_timer_vec, IDT_INT_GATE, ISR(lapic_tmr_isr),
            IST_SCHED);
    }

    /* Ensure the LAPIC base is valid */
    if (g_lapic_base == 0) {
        panic("Invalid LAPIC base\n");
    }

    ci->has_x2apic = lapic_has_x2apic();
    lapic_enable(ci);

    ci->apicid = lapic_read_id(ci);
    ci->lapic_tmr_freq = lapic_timer_init();
    bsp_trace("BSP LAPIC enabled in %s mode (id=%d)\n",
        ci->has_x2apic ? "x2APIC" : "xAPIC", ci->apicid);

    /* Try to register the timer */
    lapic_timer.name = "LAPIC_INTEGRATED_TIMER";
    lapic_timer.stop = lapic_timer_stop;
    lapic_timer.oneshot_us = lapic_timer_oneshot_us;
    register_timer(TIMER_SCHED, &lapic_timer);
}

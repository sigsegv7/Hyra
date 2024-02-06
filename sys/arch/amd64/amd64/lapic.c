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

#include <machine/lapic.h>
#include <machine/lapicvar.h>
#include <machine/cpuid.h>
#include <machine/msr.h>
#include <machine/cpu.h>
#include <machine/idt.h>
#include <machine/intr.h>
#include <machine/sysvec.h>
#include <machine/tss.h>
#include <machine/isa/i8254.h>
#include <vm/vm.h>
#include <sys/cdefs.h>
#include <sys/timer.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/mmio.h>
#include <dev/timer/hpet.h>

__MODULE_NAME("lapic");
__KERNEL_META("$Hyra$: lapic.c, Ian Marco Moffett, "
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

static struct timer lapic_timer = { 0 };

__naked void lapic_tmr_isr(void);

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
static inline uint64_t
lapic_readl(uint32_t reg)
{
    void *addr;
    const struct cpu_info *ci = this_cpu();

    if (!ci->has_x2apic) {
        addr = (void *)((uintptr_t)ci->lapic_base + reg);
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
        addr = (void *)((uintptr_t)ci->lapic_base + reg);
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
    size_t freq;

    lapic_timer_init(&freq);
    return freq;
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
lapic_get_id(const struct cpu_info *ci)
{
    if (!ci->has_x2apic) {
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
has_x2apic(void)
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
lapic_set_ldr(struct cpu_info *ci)
{
    if (!ci->has_x2apic)
        lapic_writel(LAPIC_LDR, LAPIC_STARTUP_LID);
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

    tmp = (mode << 17) | (mask << 16) | SYSVEC_LAPIC_TIMER;
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
void
lapic_timer_oneshot(bool mask, uint32_t count)
{
    lapic_timer_start(mask, LVT_TMR_ONESHOT, count);
}

/*
 * Start Local APIC timer oneshot in microseconds.
 *
 * @us: Microseconds.
 */
void
lapic_timer_oneshot_us(uint32_t us)
{
    uint64_t ticks;
    struct cpu_info *ci = this_cpu();

    ticks = us * (ci->lapic_tmr_freq / 1000000);
    lapic_timer_oneshot(false, ticks);
}

/*
 * Send an Interprocessor interrupt.
 *
 * @id: Target LAPIC ID
 * @shorthand: Dest shorthand
 * @vector: Interrupt vector
 */
void
lapic_send_ipi(uint8_t id, uint8_t shorthand, uint8_t vector)
{
    const uint32_t x2APIC_IPI_SELF = 0x3F0;
    uint8_t icr_lo = vector | IPI_DEST_PHYSICAL;
    bool x2apic_supported = has_x2apic();

    if (x2apic_supported && shorthand == IPI_SHORTHAND_SELF) {
        lapic_writel(x2APIC_IPI_SELF, vector);
        return;
    }

    switch (shorthand) {
    case IPI_SHORTHAND_SELF:
        icr_lo |= (1 << 18);
        break;
    case IPI_SHORTHAND_ALL:
        icr_lo |= (2 << 18);
        break;
    case IPI_SHORTHAND_OTHERS:
        icr_lo |= (3 << 18);
        break;
    }

    /*
     * In contrast to xAPICs two 32-bit ICR registers, the x2APIC
     * ICR register is one big 64-bit register, thus requiring only
     * a single write. In xAPIC mode, the Delivery Status bit (bit 12)
     * must be polled until clear. However, in x2APIC mode, this bit
     * does not exist meaning we do not need to worry about it.
     */
    if (x2apic_supported) {
        lapic_writel(LAPIC_ICRLO, ((uint64_t)id << 32) | icr_lo);
    } else {
        lapic_writel(LAPIC_ICRHI, ((uint32_t)id << 24));
        lapic_writel(LAPIC_ICRLO, icr_lo);
        while (__TEST(lapic_readl(LAPIC_ICRLO), __BIT(12)));
    }
}

/*
 * Calibrates the Local APIC timer
 *
 * TODO: Disable interrupts and put them back in
 *       old state.
 *
 * XXX: Will unmask IRQs if masked; will restore
 *      IRQ mask state after.
 */
void
lapic_timer_init(size_t *freq_out)
{
    struct cpu_info *ci;
    bool irq_mask = is_intr_mask();
    uint16_t init_tick, final_tick;
    size_t total_ticks;
    const uint16_t SAMPLES = 0xFFFF;

    ci = this_cpu();

    if (irq_mask) {
        __ASMV("sti");
    }

    /* Stop the timer */
    lapic_timer_oneshot(true, 0);

    i8254_set_reload(SAMPLES);
    init_tick = i8254_get_count();

    lapic_writel(LAPIC_INIT_CNT, SAMPLES);
    while (lapic_readl(LAPIC_CUR_CNT) != 0);

    final_tick = i8254_get_count();
    total_ticks = init_tick - final_tick;

    /* Calculate the frequency */
    CPU_INFO_LOCK(ci);
    ci->lapic_tmr_freq = (SAMPLES / total_ticks) * i8254_DIVIDEND;
    if (freq_out != NULL) *freq_out = ci->lapic_tmr_freq;
    CPU_INFO_UNLOCK(ci);

    /* Stop timer again */
    lapic_timer_oneshot(true, 0);

    if (irq_mask) {
        __ASMV("cli");
    }
}

void
lapic_init(void)
{
    struct cpu_info *ci;
    union tss_stack tmr_stack;
    uint64_t tmp;

    if (!lapic_check_support()) {
        /*
         * Hyra currently depends on the existance
         * of a Local APIC.
         */
        panic("This machine does not support LAPIC!\n");
    }

    /* Get the current processor, and lock its structure */
    ci = this_cpu();
    CPU_INFO_LOCK(ci);

    ci->has_x2apic = has_x2apic();

    /* Sanity check */
    if (ci->lapic_base == NULL) {
        panic("LAPIC base not set!\n");
    }

    /* Hardware enable the Local APIC */
    tmp = rdmsr(IA32_APIC_BASE_MSR);
    tmp |= ci->has_x2apic << x2APIC_ENABLE_SHIFT;
    wrmsr(IA32_APIC_BASE_MSR, tmp | LAPIC_HW_ENABLE);

    /* Software enable the Local APIC via SVR */
    lapic_reg_set(LAPIC_SVR, LAPIC_SW_ENABLE);

    BSP_KINFO("Enabled Local APIC for BSP\n");
    lapic_set_ldr(ci);

    /* Setup the timer descriptor */
    lapic_timer.name = "LAPIC_INTEGRATED_TIMER";
    lapic_timer.calibrate = lapic_timer_calibrate;
    lapic_timer.stop = lapic_timer_stop;

    /* Register the timer for scheduler usage */
    register_timer(TIMER_SCHED, &lapic_timer);

    /* Set the Local APIC ID */
    ci->id = lapic_get_id(ci);
    BSP_KINFO("BSP Local APIC ID: %d\n", ci->id);

    /* Setup LAPIC Timer TSS stack */
    if (tss_alloc_stack(&tmr_stack, vm_get_page_size()) != 0) {
        panic("Failed to allocate LAPIC TMR stack! (1 page of mem)\n");
    }
    tss_update_ist(ci, tmr_stack, IST_SCHED);
    CPU_INFO_UNLOCK(ci);

    /* Calibrate timer */
    lapic_timer_init(NULL);

    /* Setup LAPIC Timer ISR */
    idt_set_desc(SYSVEC_LAPIC_TIMER, IDT_INT_GATE_FLAGS,
                 (uintptr_t)lapic_tmr_isr, IST_SCHED);

    BSP_KINFO("LAPIC Timer on Interrupt Stack %d (IST_SCHED) with vector 0x%x\n",
              IST_SCHED, SYSVEC_LAPIC_TIMER);
}

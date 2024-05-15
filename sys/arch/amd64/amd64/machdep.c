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

#include <sys/machdep.h>
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/ksyms.h>
#include <sys/timer.h>
#include <machine/trap.h>
#include <machine/idt.h>
#include <machine/io.h>
#include <machine/gdt.h>
#include <machine/ioapic.h>
#include <machine/hpet.h>
#include <machine/lapic.h>
#include <machine/tss.h>
#include <machine/spectre.h>
#include <machine/isa/spkr.h>
#include <machine/cpu.h>
#include <machine/uart.h>
#include <machine/cpuid.h>
#include <machine/sysvec.h>
#include <dev/pci/pci.h>
#include <vm/vm.h>
#include <vm/dynalloc.h>
#include <vm/physseg.h>
#include <firmware/acpi/acpi.h>
#include <sys/syslog.h>
#include <assert.h>
#include <string.h>

__MODULE_NAME("machdep");
__KERNEL_META("$Hyra$: machdep.c, Ian Marco Moffett, "
              "Core machine dependent code");

#define ISR(func) ((uintptr_t)func)
#define INIT_FLAG_IOAPIC 0x00000001U
#define INIT_FLAG_ACPI   0x00000002U

void syscall_isr(void);

__attr(interrupt)
static void
halt_isr(void *sf)
{
    processor_halt();
}

static inline void
init_tss(struct cpu_info *cur_cpu)
{
    struct tss_desc *desc;

    desc = (struct tss_desc *)g_gdt_tss;
    write_tss(cur_cpu, desc);
    tss_load();
}

static void
interrupts_init(void)
{
    idt_set_desc(0x0, IDT_TRAP_GATE_FLAGS, ISR(arith_err), 0);
    idt_set_desc(0x2, IDT_TRAP_GATE_FLAGS, ISR(nmi), 0);
    idt_set_desc(0x3, IDT_TRAP_GATE_FLAGS, ISR(breakpoint_handler), 0);
    idt_set_desc(0x4, IDT_TRAP_GATE_FLAGS, ISR(overflow), 0);
    idt_set_desc(0x5, IDT_TRAP_GATE_FLAGS, ISR(bound_range), 0);
    idt_set_desc(0x6, IDT_TRAP_GATE_FLAGS, ISR(invl_op), 0);
    idt_set_desc(0x8, IDT_TRAP_GATE_FLAGS, ISR(double_fault), 0);
    idt_set_desc(0xA, IDT_TRAP_GATE_FLAGS, ISR(invl_tss), 0);
    idt_set_desc(0xB, IDT_TRAP_GATE_FLAGS, ISR(segnp), 0);
    idt_set_desc(0xC, IDT_TRAP_GATE_FLAGS, ISR(ss_fault), 0);
    idt_set_desc(0xD, IDT_TRAP_GATE_FLAGS, ISR(general_prot), 0);
    idt_set_desc(0xE, IDT_TRAP_GATE_FLAGS, ISR(page_fault), 0);
    idt_set_desc(0x80, IDT_INT_GATE_USER, ISR(syscall_isr), 0);
    idt_set_desc(SYSVEC_HLT, IDT_INT_GATE_FLAGS, ISR(halt_isr), 0);
    idt_load();
}

static bool
is_sse_supported(void)
{
    uint32_t edx, unused;

    __CPUID(0x00000001, unused, unused, unused, edx);
    return __TEST(edx, __BIT(25)) && __TEST(edx, __BIT(26));
}

static const char*
backtrace_addr_to_name(uintptr_t addr, off_t *off)
{
    uintptr_t prev_addr = 0;
    const char *name = NULL;

    for (size_t i = 0;;) {
        if (g_ksym_table[i].addr > addr) {
            *off = addr - prev_addr;
            return name;
        }

        prev_addr = g_ksym_table[i].addr;
        name = g_ksym_table[i].name;
        if (g_ksym_table[i++].addr == (uint64_t)-1)
            break;
    }

    return NULL;
}

static void
backtrace(void)
{
    uintptr_t *rbp;
    uintptr_t rip;

    off_t off;
    const char *name;

    kprintf("** Backtrace **\n");
    __ASMV("mov %%rbp, %0" : "=r" (rbp) :: "memory");

    while (1) {
        rip = rbp[1];
        rbp = (uintptr_t *)rbp[0];
        name = backtrace_addr_to_name(rip, &off);

        if (rbp == NULL)
            break;
        if (name == NULL)
            name = "???";

        kprintf("[0x%p] <%s+0x%x>\n", rip, name, off);
    }
}

void
processor_halt(void)
{
    __ASMV("cli; hlt");
}

/*
 * Send char to serial for debugging purposes.
 */
void
serial_dbgch(char c)
{
    uart8250_write(c);
}

void
chips_init(void)
{
    /* Hyra requires HPET on x86_64 */
    if (hpet_init() != 0)
        panic("Machine does not support HPET!\n");

    pci_init();
}

/*
 * These are critical things that need to be set up as soon as possible
 * way before the processor_init() call.
 */
void
pre_init(void)
{
    static bool is_bsp = true;

    /*
     * Certain things like serial ports, virtual memory,
     * etc, need to be set up only once! These things
     * are to be done on the BSP only...
     */
    if (is_bsp) {
        is_bsp = false;

        uart8250_try_init();
        vm_physseg_init();
        vm_init();
    }
    interrupts_init();
    gdt_load(&g_gdtr);
}

void
intr_mask(void)
{
    __ASMV("cli");
}

void
intr_unmask(void)
{
    __ASMV("sti");
}

/*
 * Called last within panic()
 */
void
machine_panic(void)
{
    struct timer tmr = {0};
    bool has_timer = true;

    if (req_timer(TIMER_GP, &tmr) != TMRR_SUCCESS)
        has_timer = false;
    if (tmr.msleep == NULL)
        has_timer = false;

    backtrace();

    /*
     * If we can use the timer, beep twice to
     * alert the user.
     */
    if (has_timer) {
        for (int i = 0; i < 2; ++i) {
            pcspkr_tone(1050, 200);
            tmr.msleep(100);
        }
    }

    processor_halt();
}

void
cpu_halt_others(void)
{
    struct cpu_info *ci = this_cpu();

    /* Is the current CPU structure even valid? */
    if (ci == NULL)
        return;

    /*
     * If the Local APIC base address is NULL, we can't
     * use lapic_send_ipi() but we we can assume the APs
     * are still parked.
     */
    if (ci->lapic_base != NULL)
        lapic_send_ipi(0, IPI_SHORTHAND_OTHERS, SYSVEC_HLT);
}

int
processor_init_pcb(struct proc *proc)
{
    struct pcb *pcb = &proc->pcb;
    const uint16_t FPU_FCW = 0x33F;
    const uint32_t SSE_MXCSR = 0x1F80;

    /* Allocate FPU save area, aligned on a 16 byte boundary */
    pcb->fpu_state = PHYS_TO_VIRT(vm_alloc_pageframe(1));
    if (pcb->fpu_state == NULL) {
        return -1;
    }

    /*
     * Setup x87 FPU control word and SSE MXCSR bits
     * as per the sysv ABI
     */
    __ASMV("fldcw %0\n"
           "ldmxcsr %1"
           :: "m" (FPU_FCW),
              "m" (SSE_MXCSR) : "memory");

    amd64_fxsave(pcb->fpu_state);
    return 0;
}

int
processor_free_pcb(struct proc *proc)
{
    struct pcb *pcb = &proc->pcb;

    if (pcb->fpu_state == NULL) {
        return -1;
    }

    vm_free_pageframe(VIRT_TO_PHYS(pcb->fpu_state), 1);
    return 0;
}

void
processor_switch_to(struct proc *old_td, struct proc *new_td)
{
    struct pcb *old_pcb = (old_td != NULL) ? &old_td->pcb : NULL;
    struct pcb *new_pcb = &new_td->pcb;

    if (old_pcb != NULL) {
        amd64_fxsave(old_pcb->fpu_state);
    }
    amd64_fxrstor(new_pcb->fpu_state);
}

void
processor_init(void)
{
    /* Indicates what doesn't need to be init anymore */
    static uint8_t init_flags = 0;
    static uint64_t reg_tmp;
    struct cpu_info *cur_cpu;

    /* Create our cpu_info structure */
    cur_cpu = dynalloc(sizeof(struct cpu_info));
    __assert(cur_cpu != NULL);
    memset(cur_cpu, 0, sizeof(struct cpu_info));

    /* Set %GS to cpu_info */
    amd64_write_gs_base((uintptr_t)cur_cpu);

    /* Try to enable SSE/SSE2 */
    if (is_sse_supported()) {
        reg_tmp = amd64_read_cr0();
        reg_tmp &= ~(__BIT(2));
        reg_tmp |= __BIT(1);
        amd64_write_cr0(reg_tmp);

        /* Enable FXSAVE/FXRSTOR */
        reg_tmp = amd64_read_cr4();
        reg_tmp |= 3 << 9;
        amd64_write_cr4(reg_tmp);
    } else {
        panic("SSE/SSE2 not supported!\n");
    }

    CPU_INFO_LOCK(cur_cpu);
    init_tss(cur_cpu);

    /* Set up some ACPI things */
    if (!__TEST(init_flags, INIT_FLAG_ACPI)) {
        /*
         * Parse the MADT... This is needed to fetch required information
         * to set up the Local APIC(s) and I/O APIC(s)...
         */
        init_flags |= INIT_FLAG_ACPI;
        acpi_parse_madt(cur_cpu);
    }

    /* Setup I/O APIC */
    if (!__TEST(init_flags, INIT_FLAG_IOAPIC)) {
        init_flags |= INIT_FLAG_IOAPIC;
        ioapic_init();
    }

    cur_cpu->lapic_base = acpi_get_lapic_base();
    CPU_INFO_UNLOCK(cur_cpu);
    lapic_init();

    /* Use spectre mitigation if enabled */
    if (try_spectre_mitigate != NULL)
        try_spectre_mitigate();

    __ASMV("sti");
}

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
#include <machine/trap.h>
#include <machine/idt.h>
#include <machine/gdt.h>
#include <machine/ioapic.h>
#include <machine/hpet.h>
#include <machine/lapic.h>
#include <machine/tss.h>
#include <machine/spectre.h>
#include <machine/cpu.h>
#include <machine/uart.h>
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
    idt_set_desc(0xD, IDT_TRAP_GATE_FLAGS, ISR(general_prot), 0);
    idt_set_desc(0xE, IDT_TRAP_GATE_FLAGS, ISR(page_fault), 0);
    idt_load();
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
processor_init(void)
{
    /* Indicates what doesn't need to be init anymore */
    static uint8_t init_flags = 0;
    struct cpu_info *cur_cpu;

    /* Create our cpu_info structure */
    cur_cpu = dynalloc(sizeof(struct cpu_info));
    __assert(cur_cpu != NULL);
    memset(cur_cpu, 0, sizeof(struct cpu_info));

    /* Set %GS to cpu_info */
    amd64_write_gs_base((uintptr_t)cur_cpu);

    CPU_INFO_LOCK(cur_cpu);
    init_tss(cur_cpu);

    /*
     * See if there are things that have not been set up,
     * set them up if so...
     */
    if (!__TEST(init_flags, INIT_FLAG_ACPI)) {
        /*
         * Parse the MADT... This is needed to fetch required information
         * to set up the Local APIC(s) and I/O APIC(s)...
         */
        init_flags |= INIT_FLAG_ACPI;
        acpi_parse_madt(cur_cpu);
    }
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

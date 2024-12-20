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

#include <sys/types.h>
#include <sys/syslog.h>
#include <sys/ksyms.h>
#include <sys/panic.h>
#include <machine/cpu.h>
#include <machine/gdt.h>
#include <machine/tss.h>
#include <machine/idt.h>
#include <machine/trap.h>
#include <machine/asm.h>
#include <machine/cpuid.h>
#include <machine/lapic.h>
#include <machine/uart.h>
#include <machine/intr.h>

#if defined(__SPECTRE_IBRS)
#define SPECTRE_IBRS  __SPECTRE_IBRS
#else
#define SPECTRE_IBRS 0
#endif

static uint8_t halt_vector = 0;

int ibrs_enable(void);
void syscall_isr(void);

struct cpu_info g_bsp_ci = {0};
static struct gdtr bsp_gdtr = {
    .limit = sizeof(struct gdt_entry) * 256 - 1,
    .offset = (uintptr_t)&g_gdt_data[0]
};

__attribute__((__interrupt__))
static void
cpu_halt_isr(void *p)
{
    __ASMV("cli; hlt");
    __builtin_unreachable();
}

static void
setup_vectors(void)
{
    if (halt_vector == 0) {
        halt_vector = intr_alloc_vector("cpu-halt", IPL_HIGH);
    }

    idt_set_desc(0x0, IDT_TRAP_GATE, ISR(arith_err), 0);
    idt_set_desc(0x2, IDT_TRAP_GATE, ISR(nmi), 0);
    idt_set_desc(0x3, IDT_TRAP_GATE, ISR(breakpoint_handler), 0);
    idt_set_desc(0x4, IDT_TRAP_GATE, ISR(overflow), 0);
    idt_set_desc(0x5, IDT_TRAP_GATE, ISR(bound_range), 0);
    idt_set_desc(0x6, IDT_TRAP_GATE, ISR(invl_op), 0);
    idt_set_desc(0x8, IDT_TRAP_GATE, ISR(double_fault), 0);
    idt_set_desc(0xA, IDT_TRAP_GATE, ISR(invl_tss), 0);
    idt_set_desc(0xB, IDT_TRAP_GATE, ISR(segnp), 0);
    idt_set_desc(0xC, IDT_TRAP_GATE, ISR(ss_fault), 0);
    idt_set_desc(0xD, IDT_TRAP_GATE, ISR(general_prot), 0);
    idt_set_desc(0xE, IDT_TRAP_GATE, ISR(page_fault), 0);
    idt_set_desc(0x80, IDT_USER_INT_GATE, ISR(syscall_isr), 0);
    idt_set_desc(halt_vector, IDT_INT_GATE, ISR(cpu_halt_isr), 0);
}

static inline void
init_tss(struct cpu_info *ci)
{
    struct tss_desc *desc;

    desc = (struct tss_desc *)&g_gdt_data[GDT_TSS];
    write_tss(ci, desc);
    tss_load();
}

static void
try_mitigate_spectre(void)
{
    if (!SPECTRE_IBRS) {
        return;
    }

    ibrs_enable();
}

static const char *
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

void
md_backtrace(void)
{
    uintptr_t *rbp;
    uintptr_t rip;
    off_t off;
    const char *name;

    __ASMV("mov %%rbp, %0" : "=r" (rbp) :: "memory");
    while (1) {
        rip = rbp[1];
        rbp = (uintptr_t *)rbp[0];
        name = backtrace_addr_to_name(rip, &off);

        if (rbp == NULL)
            break;
        if (name == NULL)
            name = "???";

        kprintf(OMIT_TIMESTAMP "%p  @ <%s+0x%x>\n", rip, name, off);
    }
}

void
cpu_halt_all(void)
{
    /*
     * If we have no current 'cpu_info' structure set,
     * we can't send IPIs, so just assume only the current
     * processor is the only one active, clear interrupts
     * then halt it.
     */
    if (rdmsr(IA32_GS_BASE) == 0) {
        __ASMV("cli; hlt");
    }

    /* Send IPI to all cores */
    lapic_send_ipi(0, IPI_SHORTHAND_ALL, halt_vector);
    for (;;);
}

void
serial_init(void)
{
    uart_init();
}

void
serial_putc(char c)
{
   uart_write(c);
}

/*
 * Get the descriptor for the currently
 * running processor.
 */
struct cpu_info *
this_cpu(void)
{
    struct cpu_info *ci;

    /*
     * This might look crazy but we are just leveraging the "m"
     * constraint to add the offset of the self field within
     * cpu_info. The self field points to the cpu_info structure
     * itself allowing us to access cpu_info through %gs.
     */
    __ASMV("mov %%gs:%1, %0"
        : "=r" (ci)
        : "m" (*&((struct cpu_info *)0)->self)
        : "memory");

    return ci;
}

void
cpu_startup(struct cpu_info *ci)
{
    ci->self = ci;
    gdt_load(&bsp_gdtr);
    idt_load();

    setup_vectors();
    wrmsr(IA32_GS_BASE, (uintptr_t)ci);

    init_tss(ci);
    try_mitigate_spectre();

    __ASMV("sti");              /* Unmask interrupts */
    lapic_init();
}

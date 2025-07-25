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
#include <machine/sync.h>
#include <machine/intr.h>
#include <machine/cdefs.h>
#include <machine/isa/i8042var.h>
#include <dev/cons/cons.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("cpu: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)
#define pr_trace_bsp(...)       \
    if (!bsp_init) {            \
        pr_trace(__VA_ARGS__);  \
    }

#define HALT_VECTOR 0x21
#define TLB_VECTOR  0x22

#if defined(__SPECTRE_IBRS)
#define SPECTRE_IBRS  __SPECTRE_IBRS
#else
#define SPECTRE_IBRS 0
#endif

#if defined(__CPU_SMEP)
#define CPU_SMEP __CPU_SMEP
#else
#define CPU_SMEP 0
#endif

int ibrs_enable(void);
int simd_init(void);
void syscall_isr(void);
void pin_isr_load(void);

struct cpu_info g_bsp_ci = {0};
static bool bsp_init = false;

__attribute__((__interrupt__))
static void
cpu_halt_isr(void *p)
{
    __ASMV("cli; hlt");
    __builtin_unreachable();
}

__attribute__((__interrupt__))
static void
tlb_shootdown_isr(void *p)
{
    struct cpu_info *ci;
    int ipl;

    /*
     * Get the current CPU and check if we even
     * need a shootdown. If `tlb_shootdown' is
     * unset, this is not for us.
     */
    ci = this_cpu();
    if (!ci->tlb_shootdown) {
        return;
    }

    ipl = splraise(IPL_HIGH);
    __invlpg(ci->shootdown_va);

    ci->shootdown_va = 0;
    ci->tlb_shootdown = 0;
    splx(ipl);
}

static void
setup_vectors(struct cpu_info *ci)
{
    union tss_stack scstack;

    /* Try to allocate a syscall stack */
    if (tss_alloc_stack(&scstack, DEFAULT_PAGESIZE) != 0) {
        panic("failed to allocate syscall stack\n");
    }

    tss_update_ist(ci, scstack, IST_SYSCALL);
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
    idt_set_desc(0x80, IDT_USER_INT_GATE, ISR(syscall_isr), IST_SYSCALL);
    idt_set_desc(HALT_VECTOR, IDT_INT_GATE, ISR(cpu_halt_isr), 0);
    idt_set_desc(TLB_VECTOR, IDT_INT_GATE, ISR(tlb_shootdown_isr), 0);
    pin_isr_load();
}

static inline void
init_tss(struct cpu_info *ci)
{
    struct tss_desc *desc;

    desc = (struct tss_desc *)&g_gdt_data[GDT_TSS_INDEX];
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

static void
enable_simd(void)
{
    int retval;

    if ((retval = simd_init()) < 0) {
        pr_trace_bsp("SIMD not supported\n");
    }

    if (retval == 1) {
        pr_trace_bsp("SSE enabled but not AVX\n");
    }
}

static void
cpu_get_info(struct cpu_info *ci)
{
    uint32_t eax, ebx, unused;
    uint8_t ext_model, ext_family;

    /* Extended features */
    CPUID(0x07, unused, ebx, unused, unused);
    if (ISSET(ebx, BIT(7)))
        ci->feat |= CPU_FEAT_SMEP;
    if (ISSET(ebx, BIT(20)))
        ci->feat |= CPU_FEAT_SMAP;

    /*
     * Processor info and feature bits
     */
    CPUID(0x01, eax, unused, unused, unused);
    ci->model = (eax >> 4) & 0xF;
    ci->family = (eax >> 8) & 0xF;

    /*
     * If the family ID is 15 then the actual family
     * ID is the sum of the extended family and the
     * family ID fields.
     */
    if (ci->family == 0xF) {
        ext_family = (eax >> 20) & 0xFF;
        ci->family += ext_family;
    }

    /*
     * If the family has the value of either 6 or 15,
     * then the extended model number would be used.
     * Slap them together if this is the case.
     */
    if (ci->family == 6 || ci->family == 15) {
        ext_model = (eax >> 16) & 0xF;
        ci->model |= (ext_model << 4);
    }
}

void
cpu_shootdown_tlb(vaddr_t va)
{
    uint32_t ncpu = cpu_count();
    struct cpu_info *cip;

    for (uint32_t i = 0; i < ncpu; ++i) {
        cip = cpu_get(i);
        if (cip == NULL) {
            break;
        }

        spinlock_acquire(&cip->lock);
        cip->shootdown_va = va;
        cip->tlb_shootdown = 1;
        lapic_send_ipi(cip->apicid, IPI_SHORTHAND_NONE, TLB_VECTOR);
        spinlock_release(&cip->lock);
    }
}

void
md_backtrace(void)
{
    uintptr_t *rbp;
    uintptr_t rip;
    off_t off;
    const char *name;
    char line[256];

    __ASMV("mov %%rbp, %0" : "=r" (rbp) :: "memory");
    while (1) {
        rip = rbp[1];
        rbp = (uintptr_t *)rbp[0];
        name = backtrace_addr_to_name(rip, &off);

        if (rbp == NULL)
            break;
        if (name == NULL)
            name = "???";

        snprintf(line, sizeof(line), "%p @ <%s+0x%x>\n", rip, name, off);
        cons_putstr(&g_root_scr, line, strlen(line));
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
    lapic_send_ipi(0, IPI_SHORTHAND_ALL, HALT_VECTOR);
    for (;;);
}

/*
 * Same as cpu_halt_all() but for all other
 * cores but ourselves.
 */
void
cpu_halt_others(void)
{
    if (rdmsr(IA32_GS_BASE) == 0) {
        __ASMV("cli; hlt");
    }

    /* Send IPI to all cores */
    lapic_send_ipi(0, IPI_SHORTHAND_OTHERS, HALT_VECTOR);
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

    if (rdmsr(IA32_GS_BASE) == 0) {
        return NULL;
    }

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

/*
 * Sync all system operation
 */
int
md_sync_all(void)
{
    lapic_eoi();
    i8042_sync();
    return 0;
}

void
cpu_enable_smep(void)
{
    struct cpu_info *ci;
    uint64_t cr4;

    /* Don't bother if not enabled */
    if (!CPU_SMEP) {
        return;
    }

    ci = this_cpu();
    if (!ISSET(ci->feat, CPU_FEAT_SMEP)) {
        pr_trace_bsp("SMEP not supported\n");
        return;
    }

    cr4 = amd64_read_cr4();
    cr4 |= BIT(20);         /* CR4.SMEP */
    amd64_write_cr4(cr4);
}

void
cpu_disable_smep(void)
{
    struct cpu_info *ci;
    uint64_t cr4;

    if (!CPU_SMEP) {
        return;
    }

    ci = this_cpu();
    if (!ISSET(ci->feat, CPU_FEAT_SMEP)) {
        return;
    }

    cr4 = amd64_read_cr4();
    cr4 &= ~BIT(20);    /* CR4.SMEP */
    amd64_write_cr4(cr4);
}

void
cpu_startup(struct cpu_info *ci)
{
    ci->self = ci;
    ci->feat = 0;
    gdt_load();
    idt_load();

    wrmsr(IA32_GS_BASE, (uintptr_t)ci);
    init_tss(ci);
    setup_vectors(ci);

    try_mitigate_spectre();
    ci->online = 1;

    cpu_get_info(ci);
    cpu_enable_smep();

    enable_simd();
    lapic_init();

    if (!bsp_init) {
        bsp_init = true;
    }
}

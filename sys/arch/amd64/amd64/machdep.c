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
#include <machine/ipi.h>
#include <machine/cdefs.h>
#include <machine/isa/i8042var.h>
#include <dev/cons/cons.h>
#include <string.h>

/*
 * This defines the max number of frames
 * we will pass while walking the callstack
 * in md_backtrace()
 */
#define MAX_FRAME_DEPTH 16

#define pr_trace(fmt, ...) kprintf("cpu: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)
#define pr_trace_bsp(...)       \
    if (!bsp_init) {            \
        pr_trace(__VA_ARGS__);  \
    }

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

#if defined(__CPU_UMIP)
#define CPU_UMIP __CPU_UMIP
#else
#define CPU_UMIP 0
#endif

int ibrs_enable(void);
int simd_init(void);
void syscall_isr(void);
void pin_isr_load(void);

struct cpu_info g_bsp_ci = {0};
static struct cpu_ipi *halt_ipi;
static struct cpu_ipi *tlb_ipi;
static struct spinlock ipi_lock = {0};
static bool bsp_init = false;

static int
cpu_halt_handler(struct cpu_ipi *ipi)
{
    __ASMV("cli; hlt");
    __builtin_unreachable();
}

static int
tlb_shootdown_handler(struct cpu_ipi *ipi)
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
        return -1;
    }

    ipl = splraise(IPL_HIGH);
    __invlpg(ci->shootdown_va);

    ci->shootdown_va = 0;
    ci->tlb_shootdown = 0;
    splx(ipl);
    return 0;
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
init_ipis(void)
{
    int error;

    if (bsp_init) {
        return;
    }

    spinlock_acquire(&ipi_lock);
    error = md_ipi_alloc(&halt_ipi);
    if (error < 0) {
        pr_error("md_ipi_alloc: returned %d\n", error);
        panic("failed to init halt IPI\n");
    }

    halt_ipi->handler = cpu_halt_handler;
    error = md_ipi_alloc(&tlb_ipi);
    if (error < 0) {
        pr_error("md_ipi_alloc: returned %d\n", error);
        panic("failed to init TLB IPI\n");
    }

    tlb_ipi->handler = tlb_shootdown_handler;

    /*
     * Some IPIs must have very specific IDs
     * so that they are standard and usable
     * throughout the rest of the sytem.
     */
    if (halt_ipi->id != IPI_HALT)
        panic("expected IPI_HALT for halt IPI\n");
    if (tlb_ipi->id != IPI_TLB)
        panic("expected IPI_TLB for TLB IPI\n");

    spinlock_release(&ipi_lock);
}

static void
cpu_get_vendor(struct cpu_info *ci)
{
    uint32_t unused, ebx, ecx, edx;
    char vendor_str[13];

    /*
     * This CPUID returns a 12 byte CPU vendor string
     * that we'll put together and use to detect the vendor.
     */
    CPUID(0, unused, ebx, ecx, edx);

    /* Dword 0 */
    vendor_str[0] = ebx & 0xFF;
    vendor_str[1] = (ebx >> 8) & 0xFF;
    vendor_str[2] = (ebx >> 16) & 0xFF;
    vendor_str[3] = (ebx >> 24) & 0xFF;

    /* Dword 1 */
    vendor_str[4] = edx & 0xFF;
    vendor_str[5] = (edx >> 8) & 0xFF;
    vendor_str[6] = (edx >> 16) & 0xFF;
    vendor_str[7] = (edx >> 24) & 0xFF;

    /* Dword 2 */
    vendor_str[8] = ecx & 0xFF;
    vendor_str[9] = (ecx >> 8) & 0xFF;
    vendor_str[10] = (ecx >> 16) & 0xFF;
    vendor_str[11] = (ecx >> 24) & 0xFF;
    vendor_str[12] = '\0';

    /* Is this an AMD CPU? */
    if (strcmp(vendor_str, "AuthenticAMD") == 0) {
        ci->vendor = CPU_VENDOR_AMD;
        return;
    }

    /* Is this an Intel CPU? */
    if (strcmp(vendor_str, "GenuineIntel") == 0) {
        ci->vendor = CPU_VENDOR_INTEL;
        return;
    }

    /*
     * Some buggy Intel CPUs report the string "GenuineIotel"
     * instead of "GenuineIntel". This is rare but we should
     * still handle it as it can happen. Probably a good idea
     * to log it so the user can know about their rare CPU
     * quirk and brag to their friends :~)
     */
    if (strcmp(vendor_str, "GenuineIotel") == 0) {
        pr_trace_bsp("vendor_str=%s\n", vendor_str);
        pr_trace_bsp("detected vendor string quirk\n");
        ci->vendor = CPU_VENDOR_INTEL;
        return;
    }

    ci->vendor = CPU_VENDOR_OTHER;
}

static void
cpu_get_info(struct cpu_info *ci)
{
    uint32_t eax, ebx, ecx, unused;
    uint8_t ext_model, ext_family;

    /* Get the vendor information */
    cpu_get_vendor(ci);

    /* Extended features */
    CPUID(0x07, unused, ebx, ecx, unused);
    if (ISSET(ebx, BIT(7)))
        ci->feat |= CPU_FEAT_SMEP;
    if (ISSET(ebx, BIT(20)))
        ci->feat |= CPU_FEAT_SMAP;
    if (ISSET(ecx, BIT(2)))
        ci->feat |= CPU_FEAT_UMIP;

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

/*
 * The CR4.UMIP bit prevents user programs from
 * executing instructions related to accessing
 * system memory structures. This should be enabled
 * by default if supported.
 */
static void
cpu_enable_umip(void)
{
    struct cpu_info *ci = this_cpu();
    uint64_t cr4;

    if (!CPU_UMIP) {
        pr_trace_bsp("UMIP not configured\n");
        return;
    }

    if (ISSET(ci->feat, CPU_FEAT_UMIP)) {
        cr4 = amd64_read_cr4();
        cr4 |= CR4_UMIP;
        amd64_write_cr4(cr4);
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
        cpu_ipi_send(cip, IPI_TLB);
        spinlock_release(&cip->lock);
    }
}

void
md_backtrace(void)
{
    uintptr_t *rbp;
    uintptr_t rip, tmp;
    off_t off;
    const char *name;
    char line[256];
    uint8_t n = 0;

    __ASMV("mov %%rbp, %0" : "=r" (rbp) :: "memory");
    while (1) {
        if (n >= MAX_FRAME_DEPTH) {
            break;
        }

        rip = rbp[1];
        rbp = (uintptr_t *)rbp[0];

        /*
         * RBP should be aligned on an 8-byte
         * boundary... Don't trust this state
         * anymore if it is not.
         */
        tmp = (uintptr_t)rbp;
        if ((tmp & (8 - 1)) != 0) {
            break;
        }

        /*
         * This is not a valid value, get out
         * of this loop!!
         */
        if (rbp == NULL || rip == 0) {
            break;
        }

        name = backtrace_addr_to_name(rip, &off);
        snprintf(line, sizeof(line), "%p @ <%s+0x%x>\n", rip, name, off);
        cons_putstr(&g_root_scr, line, strlen(line));
        ++n;
    }
}

void
cpu_halt_all(void)
{
    struct cpu_info *ci;
    uint32_t ncpu;

    /*
     * If we have no current 'cpu_info' structure set,
     * we can't send IPIs, so just assume only the current
     * processor is the only one active, clear interrupts
     * then halt it.
     */
    if (rdmsr(IA32_GS_BASE) == 0) {
        __ASMV("cli; hlt");
    }

    for (int i = 0; i < ncpu; ++i) {
        ci = cpu_get(i);
        if (ci == NULL) {
            continue;
        }

        cpu_ipi_send(ci, IPI_HALT);
    }

    for (;;);
}

/*
 * Same as cpu_halt_all() but for all other
 * cores but ourselves.
 */
void
cpu_halt_others(void)
{
    struct cpu_info *curcpu, *ci;
    uint32_t ncpu;

    if (rdmsr(IA32_GS_BASE) == 0) {
        __ASMV("cli; hlt");
    }

    curcpu = this_cpu();
    ncpu = cpu_count();

    for (int i = 0; i < ncpu; ++i) {
        if ((ci = cpu_get(i)) == NULL)
            continue;
        if (ci->id == curcpu->id)
            continue;

        cpu_ipi_send(ci, IPI_HALT);
    }
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
    md_ipi_init();
    init_ipis();

    try_mitigate_spectre();
    ci->online = 1;

    cpu_get_info(ci);
    cpu_enable_smep();
    cpu_enable_umip();

    enable_simd();
    lapic_init();

    if (!bsp_init) {
        bsp_init = true;
    }
}

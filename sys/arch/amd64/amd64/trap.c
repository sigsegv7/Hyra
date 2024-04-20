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

#include <machine/trap.h>
#include <sys/cdefs.h>
#include <sys/spinlock.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <vm/fault.h>

static const char *trap_type[] = {
    [TRAP_BREAKPOINT]   = "breakpoint",
    [TRAP_ARITH_ERR]    = "arithmetic error",
    [TRAP_OVERFLOW]     = "overflow",
    [TRAP_BOUND_RANGE]  = "bound range exceeded",
    [TRAP_INVLOP]       = "invalid opcode",
    [TRAP_DOUBLE_FAULT] = "double fault",
    [TRAP_INVLTSS]      = "invalid TSS",
    [TRAP_SEGNP]        = "segment not present",
    [TRAP_PROTFLT]      = "general protection",
    [TRAP_PAGEFLT]      = "page fault",
    [TRAP_NMI]          = "non-maskable interrupt",
    [TRAP_SS]           = "stack-segment fault"
};

static const int TRAP_COUNT = __ARRAY_COUNT(trap_type);


static inline vaddr_t
pf_faultaddr(void)
{
    uintptr_t cr2;
    __ASMV("mov %%cr2, %0\n" : "=r" (cr2) :: "memory");
    return cr2;
}

static inline vm_prot_t
pf_accesstype(struct trapframe *tf)
{
    vm_prot_t prot = 0;
    uint64_t ec = tf->error_code;

    if (__TEST(ec, __BIT(1)))
        prot |= PROT_WRITE;
    if (__TEST(ec, __BIT(2)))
        prot |= PROT_USER;
    if (__TEST(ec, __BIT(4)))
        prot |= PROT_EXEC;

    return prot;
}

static void
dbg_errcode(struct trapframe *tf)
{
    uint64_t ec = tf->error_code;

    switch (tf->trapno) {
    case TRAP_PAGEFLT:
        kprintf("bits (pwui): %c%c%c%c\n",
                __TEST(ec, __BIT(0)) ? 'p' : '-',
                __TEST(ec, __BIT(1)) ? 'w' : '-',
                __TEST(ec, __BIT(2)) ? 'u' : '-',
                __TEST(ec, __BIT(4)) ? 'i' : '-');
        break;
    case TRAP_SS:
        kprintf("ss: 0x%x\n", ec);
        break;
    }
}

static void
trap_print(struct trapframe *tf)
{
    if (tf->trapno < TRAP_COUNT) {
        kprintf("** Fatal %s **\n", trap_type[tf->trapno]);
    } else {
        kprintf("** Unknown trap %d **\n", tf->trapno);
    }

    dbg_errcode(tf);
}

static void
regdump(struct trapframe *tf)
{
    uintptr_t cr3, cr2 = pf_faultaddr();

    __ASMV("mov %%cr3, %0\n"
           : "=r" (cr3)
           :
           : "memory"
    );

    kprintf("RAX=%p RCX=%p RDX=%p\n"
            "RBX=%p RSI=%p RDI=%p\n"
            "RFL=%p CR2=%p CR3=%p\n"
            "RBP=%p RSP=%p RIP=%p\n",
            tf->rax, tf->rcx, tf->rdx,
            tf->rbx, tf->rsi, tf->rdi,
            tf->rflags, cr2, cr3,
            tf->rbp, tf->rsp, tf->rip);
}

static inline void
handle_fatal(struct trapframe *tf)
{
    trap_print(tf);
    regdump(tf);
    panic("Halted\n");
}

/*
 * Handles traps.
 */
void
trap_handler(struct trapframe *tf)
{
    struct proc *curtd = this_td();
    int s;

    /*
     * XXX: Handle NMIs better. For now we just
     *      panic.
     */
    if (tf->trapno == TRAP_NMI) {
        trap_print(tf);
        kprintf("Possible hardware failure?\n");
        panic("Caught NMI; bailing out\n");
    }

    if (curtd == NULL) {
        handle_fatal(tf);
    } else if (!curtd->is_user) {
        handle_fatal(tf);
    }

    switch (tf->trapno) {
    case TRAP_ARITH_ERR:
        signal_raise(curtd, SIGFPE);
        break;
    case TRAP_PAGEFLT:
        s = vm_fault(pf_faultaddr(), pf_accesstype(tf));
        if (s != 0)
            signal_raise(curtd, SIGSEGV);
        break;
    default:
        signal_raise(curtd, SIGSEGV);
        break;
    }
}

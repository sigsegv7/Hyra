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
#include <sys/cdefs.h>
#include <sys/reboot.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <sys/syscall.h>
#include <sys/sched.h>
#include <sys/proc.h>
#include <machine/cpu.h>
#include <machine/isa/i8042var.h>
#include <machine/trap.h>
#include <machine/frame.h>
#include <machine/intr.h>

#define pr_error(fmt, ...) kprintf("trap: " fmt, ##__VA_ARGS__)

void trap_syscall(struct trapframe *tf);

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

/* Page-fault flags */
static const char pf_flags[] = {
    'p',    /* Present */
    'w',    /* Write */
    'u',    /* User */
    'r',    /* Reserved write */
    'x',    /* Instruction fetch */
    'k',    /* Protection key violation */
    's'     /* Shadow stack access */
};

static inline uintptr_t
pf_faultaddr(void)
{
    uintptr_t cr2;
    __ASMV("mov %%cr2, %0\n" : "=r" (cr2) :: "memory");
    return cr2;
}

static void
pf_code(uint64_t error_code)
{
    char tab[8] = {
        '-', '-', '-',
        '-', '-', '-',
        '-', '\0'
    };

    for (int i = 0; i < 7; ++i) {
        if (ISSET(error_code, BIT(i))) {
            tab[i] = pf_flags[i];
        }
    }
    kprintf("code=[%s]\n", tab);
}

__dead static void
trap_fatal(struct trapframe *tf)
{
    uintptr_t cr3, cr2 = pf_faultaddr();

    __ASMV("mov %%cr3, %0\n"
           : "=r" (cr3)
           :
           : "memory"
    );

    if (tf->trapno == TRAP_PAGEFLT) {
        pf_code(tf->error_code);
    }

    panic("got fatal trap\n\n"
        "-- DUMPING PROCESSOR STATE --\n"
        "RAX=%p RCX=%p RDX=%p\n"
        "RBX=%p RSI=%p RDI=%p\n"
        "RFL=%p CR2=%p CR3=%p\n"
        "RBP=%p RSP=%p RIP=%p\n\n",
        tf->rax, tf->rcx, tf->rdx,
        tf->rbx, tf->rsi, tf->rdi,
        tf->rflags, cr2, cr3,
        tf->rbp, tf->rsp, tf->rip);
}

static void
trap_user(struct trapframe *tf)
{
    struct proc *td = this_td();
    uintptr_t fault_addr;
    sigset_t sigset;

    sigemptyset(&sigset);

    switch (tf->trapno) {
    case TRAP_PROTFLT:
    case TRAP_PAGEFLT:
        if (tf->trapno == TRAP_PAGEFLT) {
            pf_code(tf->error_code);
        }
        sigaddset(&sigset, SIGSEGV);
        break;
    case TRAP_ARITH_ERR:
        sigaddset(&sigset, SIGFPE);
        break;
    default:
        kprintf("got unknown user trap %d\n", tf->trapno);
        sigaddset(&sigset, SIGKILL);
        break;
    }

    fault_addr = pf_faultaddr();
    proc_coredump(td, fault_addr);

    /*
     * Send the signal then flush the signal queue right
     * away as these types of events are critical.
     */
    sendsig(td, &sigset);
    dispatch_signals(td);
}

void
trap_syscall(struct trapframe *tf)
{
    struct syscall_args scargs = {
        .arg0 = tf->rdi,
        .arg1 = tf->rsi,
        .arg2 = tf->rdx,
        .arg3 = tf->r10,
        .arg4 = tf->r9,
        .arg5 = tf->r8,
        .tf = tf
    };

    if (tf->rax < MAX_SYSCALLS && tf->rax > 0) {
        tf->rax = g_sctab[tf->rax](&scargs);
    }
}

void
trap_handler(struct trapframe *tf)
{
    int ipl;

    ipl = splraise(IPL_HIGH);
    if (tf->trapno >= NELEM(trap_type)) {
        panic("got unknown trap %d\n", tf->trapno);
    }

    pr_error("got %s\n", trap_type[tf->trapno]);

    /* Handle traps from userland */
    if (ISSET(tf->cs, 3)) {
        splx(ipl);
        trap_user(tf);
        return;
    }

    trap_fatal(tf);
    __builtin_unreachable();
}

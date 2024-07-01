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

#include <sys/proc.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <machine/frame.h>
#include <machine/gdt.h>
#include <vm/physmem.h>
#include <vm/vm.h>
#include <vm/map.h>
#include <string.h>

void
md_td_stackinit(struct proc *td, void *stack_top, struct exec_prog *prog)
{
    uintptr_t *sp = stack_top;
    uintptr_t old_sp;
    size_t argc, envc, len;
    char **argvp = prog->argp;
    char **envp = prog->envp;
    struct auxval auxval = prog->auxval;
    struct trapframe *tfp;

    /* Copy strings */
    old_sp = (uintptr_t)sp;
    for (argc = 0; argvp[argc] != NULL; ++argc) {
        len = strlen(argvp[argc]) + 1;
        sp = (void *)((char *)sp - len);
        memcpy((char *)sp, argvp[argc], len);
    }
    for (envc = 0; envp[envc] != NULL; ++envc) {
        len = strlen(envp[envc]) + 1;
        sp = (void *)((char *)sp - len);
        memcpy((char *)sp, envp[envc], len);
    }

    /* Ensure the stack is aligned */
    sp = (void *)ALIGN_DOWN((uintptr_t)sp, 16);
    if (((argc + envc + 1) & 1) != 0)
        --sp;

    AUXVAL(sp, AT_NULL, 0x0);
    AUXVAL(sp, AT_SECURE, 0x0);
    AUXVAL(sp, AT_ENTRY, auxval.at_entry);
    AUXVAL(sp, AT_PHDR, auxval.at_phdr);
    AUXVAL(sp, AT_PHNUM, auxval.at_phnum);
    AUXVAL(sp, AT_PAGESIZE, DEFAULT_PAGESIZE);
    STACK_PUSH(sp, 0);

    /* Copy envp pointers */
    sp -= envc;
    for (int i = 0; i < envc; ++i) {
        len = strlen(envp[i]) + 1;
        old_sp -= len;
        sp[i] = (uintptr_t)old_sp - VM_HIGHER_HALF;
    }

    /* Copy argvp pointers */
    STACK_PUSH(sp, 0);
    sp -= argc;
    for (int i = 0; i < argc; ++i) {
        len = strlen(argvp[i]) + 1;
        old_sp -= len;
        sp[i] = (uintptr_t)old_sp - VM_HIGHER_HALF;
    }

    STACK_PUSH(sp, argc);
    tfp = &td->tf;
    tfp->rsp = (uintptr_t)sp - VM_HIGHER_HALF;
}

void
setregs(struct proc *td, struct exec_prog *prog, uintptr_t stack)
{
    struct trapframe *tfp = &td->tf;
    struct auxval *auxvalp = &prog->auxval;

    memset(tfp, 0, sizeof(*tfp));
    tfp->rip = auxvalp->at_entry;
    tfp->cs = USER_CS | 3;
    tfp->ss = USER_DS | 3;
    tfp->rsp = stack;
    tfp->rflags = 0x202;
}

/*
 * MD thread init code
 *
 * @p: New process.
 * @parent: Parent of new process.
 * @ip: Instruction pointer.
 */
int
md_fork(struct proc *p, struct proc *parent, uintptr_t ip)
{
    uintptr_t stack_base;
    struct trapframe *tfp;
    struct pcb *pcbp;
    uint8_t rpl = 0;
    int error;

    tfp = &p->tf;

    /* Create a new VAS for this thread */
    pcbp = &p->pcb;
    if ((error = pmap_new_vas(&pcbp->addrsp)) != 0)
        return error;

    memcpy(tfp, &parent->tf, sizeof(p->tf));

    /*
     * Kernel threads cannot be on the lower half.
     * If 'ip' is on the lower half, assume that it
     * is a userspace program and set RPL to ring 3.
     */
    if (ip < VM_HIGHER_HALF)
        rpl = 3;

    /*
     * RPL being 3 indicates that the new thread should be in
     * userland. If this is the case, use user segment selectors.
     */
    tfp->rip = ip;
    tfp->cs = (rpl == 3) ? (USER_CS | 3) : KERNEL_CS;
    tfp->ss = (rpl == 3) ? (USER_DS | 3) : KERNEL_DS;
    tfp->rflags = 0x202;

    /* Try to allocate a new stack */
    stack_base = vm_alloc_frame(PROC_STACK_PAGES);
    if (stack_base == 0)
        return -ENOMEM;

    /*
     * If RPL is 0 (kernel), adjust the stack base to the
     * higher half. If we have an RPL of 3 (user) then we'll
     * need to identity map the stack.
     */
    if (rpl == 0) {
        stack_base += VM_HIGHER_HALF;
    } else {
        vm_map(pcbp->addrsp, stack_base, stack_base,
            PROT_READ | PROT_WRITE | PROT_USER, PROC_STACK_PAGES);
    }

    p->stack_base = stack_base;
    tfp->rsp = ALIGN_DOWN((stack_base + PROC_STACK_SIZE) - 1, 16);
    return 0;
}

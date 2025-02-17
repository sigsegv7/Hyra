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

#include <sys/exec.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/signal.h>
#include <vm/vm.h>
#include <vm/map.h>
#include <vm/physmem.h>
#include <machine/pcb.h>
#include <string.h>

/*
 * Release the memory of the old stack
 *
 * @td: Target thread.
 */
static void
release_stack(struct proc *td)
{
    struct pcb *pcbp = &td->pcb;
    uintptr_t base = td->stack_base;

    if (base >= VM_HIGHER_HALF) {
        base -= VM_HIGHER_HALF;
        vm_free_frame(base, PROC_STACK_PAGES);
    } else {
        vm_unmap(pcbp->addrsp, base, PROC_STACK_SIZE);
        vm_free_frame(base, PROC_STACK_PAGES);
    }
}

int
execve(struct proc *td, const struct execve_args *args)
{
    int error;
    struct exec_prog prog;
    struct pcb *pcbp = &td->pcb;
    uintptr_t stack_top, stack;

    if (td == NULL || args == NULL)
        return -EINVAL;
    if ((error = elf64_load(args->pathname, td, &prog)) != 0)
        return error;

    /* Mark the thread as running exec */
    td->flags |= PROC_EXEC;

    /* Create the new stack */
    stack = vm_alloc_frame(PROC_STACK_PAGES);
    if (stack == 0) {
        elf_unload(td, &prog);
        return -ENOMEM;
    }

    /* Release the old stack if it exists */
    if (td->stack_base != 0)
        release_stack(td);

    /* Save program state */
    memcpy(&td->exec, &prog, sizeof(td->exec));

    /* Set new stack and map it to userspace */
    td->stack_base = stack;
    vm_map(pcbp->addrsp, td->stack_base, td->stack_base,
        (PROT_READ | PROT_WRITE | PROT_USER), PROC_STACK_SIZE);

    prog.argp = args->argv;
    prog.envp = args->envp;
    stack_top = td->stack_base + (PROC_STACK_SIZE - 1);

    /* Setup registers, signals and stack */
    md_td_stackinit(td, (void *)(stack_top + VM_HIGHER_HALF), &prog);
    setregs(td, &prog, stack_top);
    signals_init(td);

    /* Done, reset flags and start the user thread */
    td->flags &= ~PROC_EXEC;
    md_td_kick(td);
}

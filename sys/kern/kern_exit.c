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
#include <sys/sched.h>
#include <sys/syslog.h>
#include <vm/physmem.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <vm/map.h>
#include <machine/pcb.h>

#define pr_trace(fmt, ...) kprintf("exit: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static void
unload_td(struct proc *td)
{
    const struct auxval *auxvalp;
    struct exec_prog *execp;
    struct exec_range *range;
    struct pcb *pcbp;
    size_t len;

    execp = &td->exec;
    auxvalp = &execp->auxval;
    pcbp = &td->pcb;

    for (size_t i = 0; i < auxvalp->at_phnum; ++i) {
        range = &execp->loadmap[i];
        len = (range->end - range->start);

        /* Drop entries with zeroed range */
        if (range->start == 0 && range->end == 0) {
            continue;
        }

        /* Attempt to unmap the range */
        if (vm_unmap(pcbp->addrsp, range->vbase, len) != 0) {
            pr_error("Failed to unmap %p - %p (pid=%d)\n",
                range->start, range->end, td->pid);
        }

        /* Free the physical memory */
        vm_free_frame(range->start, len / DEFAULT_PAGESIZE);
    }
}

/*
 * Kill a thread and deallocate its resources.
 *
 * @td: Thread to exit
 */
int
exit1(struct proc *td)
{
    struct pcb *pcbp;
    struct proc *curtd;
    uintptr_t stack;
    pid_t target_pid, curpid;

    target_pid = td->pid;
    curtd = this_td();
    pcbp = &td->pcb;

    curpid = curtd->pid;
    stack = td->stack_base;
    td->flags |= PROC_EXITING;

    /*
     * If this is on the higher half, it is kernel
     * mapped and we need to convert it to a physical
     * address.
     */
    if (stack >= VM_HIGHER_HALF) {
        stack -= VM_HIGHER_HALF;
    }

    unload_td(td);
    vm_unmap(pcbp->addrsp, td->stack_base, PROC_STACK_SIZE);
    vm_free_frame(stack, PROC_STACK_PAGES);

    pmap_destroy_vas(pcbp->addrsp);
    dynfree(td);

    /*
     * If we are the thread exiting, reenter the scheduler
     * and do not return.
     */
    if (target_pid == curpid)
        sched_enter();

    return 0;
}

scret_t
sys_exit(struct syscall_args *scargs)
{
    exit1(this_td());
    __builtin_unreachable();
}

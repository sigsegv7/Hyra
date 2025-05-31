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

#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/syslog.h>
#include <vm/physmem.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <vm/map.h>
#include <machine/pcb.h>
#include <machine/cpu.h>

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

    sched_detach(td);
    if (ISSET(td->flags, PROC_KTD)) {
        return;
    }

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
            pr_error("failed to unmap %p - %p (pid=%d)\n",
                range->start, range->end, td->pid);
        }

        /* Free the physical memory */
        vm_free_frame(range->start, len / DEFAULT_PAGESIZE);
    }
}

void
proc_reap(struct proc *td)
{
    struct pcb *pcbp;
    uintptr_t stack;

    pcbp = &td->pcb;
    stack = td->stack_base;

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
}

/*
 * Kill a thread and deallocate its resources.
 *
 * @td: Thread to exit
 */
int
exit1(struct proc *td, int flags)
{
    struct proc *curtd, *procp;
    struct proc *parent;
    struct cpu_info *ci;
    pid_t target_pid, curpid;

    ci = this_cpu();
    target_pid = td->pid;
    curtd = this_td();

    curpid = curtd->pid;
    td->flags |= PROC_EXITING;
    parent = td->parent;

    /* If we have any children, kill them too */
    if (td->nleaves > 0) {
        TAILQ_FOREACH(procp, &td->leafq, leaf_link) {
            exit1(procp, flags);
        }
    }

    if (target_pid != curpid) {
        proc_reap(td);
    }

    /*
     * Only free the process structure if we aren't
     * being waited on, otherwise let it be so the
     * parent can examine what's left of it.
     */
    if (!ISSET(td->flags, PROC_WAITED)) {
        dynfree(td);
    } else {
        td->flags |= PROC_ZOMB;
    }

    /*
     * If we are the thread exiting, reenter the scheduler
     * and do not return.
     */
    if (target_pid == curpid) {
        ci->curtd = NULL;
        if (parent->pid == 0)
            sched_enter();
        if (td->data == NULL)
            sched_enter();

        sched_enqueue_td(parent);
        parent->flags &= ~PROC_SLEEP;
        sched_enter();
    }

    return 0;
}

/*
 * arg0: Exit status.
 */
scret_t
sys_exit(struct syscall_args *scargs)
{
    struct proc *td = this_td();

    td->data = scargs->tf;
    td->exit_status = scargs->arg0;
    exit1(td, 0);
    __builtin_unreachable();
}

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

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <sys/types.h>
#include <sys/spinlock.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <sys/exec.h>
#include <sys/filedesc.h>
#include <sys/signal.h>
#include <sys/vnode.h>
#if defined(_KERNEL)
#include <machine/frame.h>
#include <machine/pcb.h>
#include <vm/vm.h>
#endif  /* _KERNEL */

#if defined(_KERNEL)
#define PROC_STACK_PAGES 8
#define PROC_STACK_SIZE  (PROC_STACK_PAGES * DEFAULT_PAGESIZE)
#define PROC_MAX_FILEDES 256
#define PROC_SIGMAX 64

struct proc {
    pid_t pid;
    struct exec_prog exec;
    struct ksiginfo *ksig_list[PROC_SIGMAX];
    struct filedesc *fds[PROC_MAX_FILEDES];
    struct mmap_lgdr *mlgdr;
    struct vcache *vcache;
    struct spinlock vcache_lock;
    struct trapframe tf;
    struct pcb pcb;
    size_t priority;
    bool rested;
    uint32_t flags;
    uintptr_t stack_base;
    struct spinlock ksigq_lock;
    TAILQ_HEAD(, ksiginfo) ksigq;
    TAILQ_ENTRY(proc) link;
};

#define PROC_EXITING    BIT(0)  /* Exiting */
#define PROC_EXEC       BIT(1)  /* Exec called (cleared by sched) */

struct proc *this_td(void);
int md_spawn(struct proc *p, struct proc *parent, uintptr_t ip);
int spawn(struct proc *cur, int flags, void(*ip)(void), struct proc **newprocp);

void md_td_stackinit(struct proc *td, void *stack_top, struct exec_prog *prog);
__dead void md_td_kick(struct proc *td);

int fork1(struct proc *cur, int flags, void(*ip)(void), struct proc **newprocp);
int exit1(struct proc *td);
__dead scret_t sys_exit(struct syscall_args *scargs);

#endif  /* _KERNEL */
#endif  /* !_SYS_PROC_H_ */

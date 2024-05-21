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

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/filedesc.h>
#include <sys/spinlock.h>
#include <sys/loader.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <vm/vm.h>
#include <vm/map.h>

#define PROC_MAX_FDS 256
#define PROC_MAX_ADDR_RANGE 4

#define PROC_STACK_PAGES 8
#define PROC_STACK_SIZE (PROC_STACK_PAGES*vm_get_page_size())

/*
 * The PHYS_TO_VIRT/VIRT_TO_PHYS macros convert
 * addresses to lower and higher half addresses.
 * Userspace addresses are on the lower half,
 * therefore, we can just wrap over these to
 * keep things simple.
 *
 * XXX: TODO: This won't work when not identity mapping
 *            lowerhalf addresses. Once that is updated,
 *            get rid of this.
 */
#define USER_TO_KERN(user) PHYS_TO_VIRT(user)
#define KERN_TO_USER(kern) VIRT_TO_PHYS(kern)

enum {
    ADDR_RANGE_EXEC = 0,    /* Program address range */
    ADDR_RANGE_STACK        /* Stack address range */
};

/*
 * A task running on the CPU e.g., a process or
 * a thread.
 */
struct proc {
    pid_t pid;
    struct cpu_info *cpu;
    struct trapframe *tf;
    struct pcb pcb;
    struct vas addrsp;
    struct vm_range addr_range[PROC_MAX_ADDR_RANGE];
    struct spinlock lock;
    uint8_t is_user;
    uint8_t rested;
    uint32_t signal;
    uint32_t priority;
    struct filedesc *fds[PROC_MAX_FDS];
    struct spinlock mapspace_lock;
    struct vm_mapspace mapspace;
    TAILQ_ENTRY(proc) link;
};

#endif  /* !_SYS_PROC_H_ */

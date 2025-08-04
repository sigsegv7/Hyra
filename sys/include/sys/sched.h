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

#ifndef _SYS_SCHED_H_
#define _SYS_SCHED_H_

#include <sys/proc.h>
#include <sys/cdefs.h>
#include <sys/limits.h>
#include <sys/time.h>

/*
 * Scheduler CPU information
 *
 * @nswitch: Number of context switches
 * @idle: Number of milliseconds idle
 */
struct sched_cpu {
    uint32_t nswitch;
};

/*
 * Scheduler statistics
 *
 * @nproc: Number processes running
 * @ncpu: Number of CPU cores
 * @nhlt: Number of halted CPU cores
 * @quantum_usec: Scheduler quantum (microseconds)
 */
struct sched_stat {
    size_t nproc;
    uint16_t ncpu;
    uint16_t nhlt;
    uint32_t quantum_usec;
    struct sched_cpu cpus[CPU_MAX];
};

#if defined(_KERNEL)

void sched_stat(struct sched_stat *statp);
void sched_init(void);

void sched_yield(void);
void sched_suspend(struct proc *td, const struct timeval *tv);
void sched_detach(struct proc *td);

__dead void sched_enter(void);
void sched_enqueue_td(struct proc *td);

#endif  /* _KERNEL */
#endif  /* !_SYS_SCHED_H_ */

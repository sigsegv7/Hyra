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

#ifndef _RISCV64_CPU_H_
#define _RISCV64_CPU_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/spinlock.h>
#include <sys/sched_state.h>
#include <sys/queue.h>

#define this_cpu()      amd64_this_cpu()
#define get_bsp()       amd64_get_bsp()
#define is_intr_mask()  amd64_is_intr_mask()
#define CPU_INFO_LOCK(info) spinlock_acquire(&(info->lock))
#define CPU_INFO_UNLOCK(info) spinlock_release(&(info->lock))

/*
 * Info about a specific processor.
 *
 * XXX: Spinlock must be acquired outside of this module!
 *      None of these module's internal functions should
 *      acquire the spinlock themselves!
 */
struct cpu_info {
    /* Per-arch fields  */
    void *pmap;                         /* Current pmap */
    uint32_t id;
    uint32_t idx;
    struct spinlock lock;
    struct sched_state sched_state;
    TAILQ_ENTRY(cpu_info) link;
};

struct cpu_info *amd64_this_cpu(void);
struct cpu_info *amd64_get_bsp(void);

#endif  /* !_RISCV64_CPU_H_ */

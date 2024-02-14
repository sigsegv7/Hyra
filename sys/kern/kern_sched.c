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

#include <sys/sched.h>
#include <sys/sched_state.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/spinlock.h>
#include <machine/cpu.h>
#include <assert.h>

/*
 * This is the processor ready list, processors
 * (cores) that have no task assigned live here.
 *
 * Assigning a task to a core is done by popping from
 * this list. However, it must be done carefully and
 * must be serialized. You *must* acquire ci_ready_lock
 * before performing *any* operations on ci_ready_list!!!
 */
static TAILQ_HEAD(, cpu_info) ci_ready_list;
static struct spinlock ci_ready_lock = {0};

/*
 * Push a processor into the ready list.
 */
static void
sched_enqueue_ci(struct cpu_info *ci)
{
    spinlock_acquire(&ci_ready_lock);
    TAILQ_INSERT_TAIL(&ci_ready_list, ci, link);
    spinlock_release(&ci_ready_lock);
}

/*
 * Processor awaiting tasks to be assigned will be here spinning.
 */
__noreturn static void
sched_enter(void)
{
    for (;;) {
        hint_spinwait();
    }
}

/*
 * Setup scheduler related things and enqueue AP.
 */
void
sched_init_processor(struct cpu_info *ci)
{
    struct sched_state *sched_state = &ci->sched_state;
    static bool is_init = true;

    if (is_init) {
        /* Setup ready list if first call */
        TAILQ_INIT(&ci_ready_list);
        is_init = false;
    }

    TAILQ_INIT(&sched_state->queue);
    sched_enqueue_ci(ci);

    sched_enter();

    __builtin_unreachable();
}

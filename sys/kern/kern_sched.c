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

#include <sys/types.h>
#include <sys/sched.h>
#include <sys/schedvar.h>
#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <machine/frame.h>
#include <dev/timer.h>
#include <assert.h>

#define pr_trace(fmt, ...) kprintf("ksched: " fmt, ##__VA_ARGS__)

void sched_switch(struct trapframe *tf);

static sched_policy_t policy = SCHED_POLICY_RR;

/*
 * Thread ready queues - all threads ready to be
 * scheduled should be added to the toplevel queue.
 */
static struct sched_queue qlist[SCHED_NQUEUE];

/*
 * Perform timer oneshot
 */
static inline void
sched_oneshot(bool now)
{
    struct timer timer;
    size_t usec = now ? SHORT_TIMESLICE_USEC : DEFAULT_TIMESLICE_USEC;
    tmrr_status_t tmr_status;

    tmr_status = req_timer(TIMER_SCHED, &timer);
    __assert(tmr_status == TMRR_SUCCESS);

    timer.oneshot_us(usec);
}

/*
 * Perform a context switch.
 *
 * TODO
 */
void
sched_switch(struct trapframe *tf)
{
    static struct spinlock lock = {0};

    spinlock_acquire(&lock);
    spinlock_release(&lock);
    sched_oneshot(false);
}

/*
 * Main scheduler loop
 */
void
sched_enter(void)
{
    sched_oneshot(false);
    for (;;);
}

void
sched_init(void)
{
    /* Setup the queues */
    for (int i = 0; i < SCHED_NQUEUE; ++i) {
        TAILQ_INIT(&qlist[i].q);
    }

    pr_trace("Prepared %d queues (policy=0x%x)\n",
        SCHED_NQUEUE, policy);
}

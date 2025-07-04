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

#include <sys/types.h>
#include <sys/sched.h>
#include <sys/schedvar.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/atomic.h>
#include <dev/cons/cons.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/cdefs.h>
#include <vm/pmap.h>
#include <dev/timer.h>
#include <assert.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("ksched: " fmt, ##__VA_ARGS__)

void sched_switch(struct trapframe *tf);

static sched_policy_t policy = SCHED_POLICY_MLFQ;

/*
 * Thread ready queues - all threads ready to be
 * scheduled should be added to the toplevel queue.
 */
static struct sched_queue qlist[SCHED_NQUEUE];

/*
 * Thread queue lock - all operations to `qlist'
 * must be done with this lock acquired.
 */
__cacheline_aligned static struct spinlock tdq_lock = {0};

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
 * Save thread state and enqueue it back into one
 * of the ready queues.
 */
static void
sched_save_td(struct proc *td, struct trapframe *tf)
{
    /*
     * Save trapframe to process structure only
     * if PROC_EXEC is not set.
     */
    if (!ISSET(td->flags, PROC_EXEC)) {
        memcpy(&td->tf, tf, sizeof(td->tf));
    }

    sched_enqueue_td(td);
}

static struct proc *
sched_dequeue_td(void)
{
    struct sched_queue *queue;
    struct proc *td = NULL;

    spinlock_acquire(&tdq_lock);

    for (size_t i = 0; i < SCHED_NQUEUE; ++i) {
        queue = &qlist[i];
        if (TAILQ_EMPTY(&queue->q)) {
            continue;
        }

        td = TAILQ_FIRST(&queue->q);
        if (td == NULL) {
            continue;
        }

        while (ISSET(td->flags, PROC_SLEEP)) {
            td = TAILQ_NEXT(td, link);
            if (td == NULL) {
                break;
            }
        }

        if (td == NULL) {
            continue;
        }

        TAILQ_REMOVE(&queue->q, td, link);
        spinlock_release(&tdq_lock);
        return td;
    }

    /* We got nothing */
    spinlock_release(&tdq_lock);
    return NULL;
}

/*
 * Add a thread to the scheduler.
 */
void
sched_enqueue_td(struct proc *td)
{
    struct sched_queue *queue;

    spinlock_acquire(&tdq_lock);
    queue = &qlist[td->priority];

    TAILQ_INSERT_TAIL(&queue->q, td, link);
    spinlock_release(&tdq_lock);
}

/*
 * Return the currently running thread.
 */
struct proc *
this_td(void)
{
    struct cpu_info *ci;

    ci = this_cpu();
    return ci->curtd;
}

static inline void
td_pri_raise(struct proc *td)
{
    if (td->priority > 0) {
	    td->priority--;
    }
}

static inline void
td_pri_lower(struct proc *td)
{
    if (td->priority < SCHED_NQUEUE - 1) {
        td->priority++;
    }
}

static inline void
td_pri_update(struct proc *td)
{
    switch (policy) {
    case SCHED_POLICY_MLFQ:
        if (td->rested) {
            td->rested = false;
            td_pri_raise(td);
        } else {
            td_pri_lower(td);
        }

        break;
    }
}

void
sched_switch_to(struct trapframe *tf, struct proc *td)
{
    struct cpu_info *ci;
    struct pcb *pcbp;

    ci = this_cpu();

    if (tf != NULL) {
        memcpy(tf, &td->tf, sizeof(*tf));
    }

    ci->curtd = td;
    pcbp = &td->pcb;
    pmap_switch_vas(pcbp->addrsp);
}

/*
 * Perform a context switch.
 */
void
sched_switch(struct trapframe *tf)
{
    struct proc *next_td, *td;
    struct cpu_info *ci;

    ci = this_cpu();
    td = ci->curtd;
    cons_detach();

    if (td != NULL) {
        if (td->pid == 0)
            return;

        dispatch_signals(td);
        td_pri_update(td);
        sched_save_td(td, tf);
    }

    if ((next_td = sched_dequeue_td()) == NULL) {
        sched_oneshot(false);
        return;
    }

    sched_switch_to(tf, next_td);
    sched_oneshot(false);
}

/*
 * Main scheduler loop
 */
void
sched_enter(void)
{
    md_inton();
    sched_oneshot(false);
    for (;;) {
        md_pause();
    }
}

void
sched_yield(void)
{
    struct proc *td;
    struct cpu_info *ci = this_cpu();

    if ((td = ci->curtd) == NULL) {
        return;
    }

    td->rested = true;

    /* FIXME: Hang yielding when waited on */
    if (ISSET(td->flags, PROC_WAITED)) {
        return;
    }

    ci->curtd = NULL;
    md_inton();
    sched_oneshot(false);

    md_hlt();
    md_intoff();
    ci->curtd = td;
}

void
sched_detach(struct proc *td)
{
    struct sched_queue *queue;

    spinlock_acquire(&tdq_lock);
    queue = &qlist[td->priority];

    TAILQ_REMOVE(&queue->q, td, link);
    spinlock_release(&tdq_lock);
}

void
sched_init(void)
{
    /* Setup the queues */
    for (int i = 0; i < SCHED_NQUEUE; ++i) {
        TAILQ_INIT(&qlist[i].q);
    }

    pr_trace("prepared %d queues (policy=0x%x)\n",
        SCHED_NQUEUE, policy);
}

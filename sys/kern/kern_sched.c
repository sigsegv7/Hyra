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
#include <sys/schedvar.h>
#include <sys/sched_state.h>
#include <sys/types.h>
#include <sys/timer.h>
#include <sys/cdefs.h>
#include <sys/spinlock.h>
#include <vm/dynalloc.h>
#include <assert.h>
#include <string.h>

/*
 * Thread ready queue - all threads ready to be
 * scheduled should be added to this queue.
 */
static TAILQ_HEAD(, proc) td_queue;
static size_t nthread = 0;

/*
 * Thread queue lock - all operations to `td_queue'
 * must be done with this lock acquired.
 */
static struct spinlock tdq_lock = {0};

/* In sys/<machine>/<machine>/switch.S */
void __sched_switch_to(struct trapframe *tf);

static inline void
sched_oneshot(void)
{
    struct timer timer;
    tmrr_status_t tmr_status;

    tmr_status = req_timer(TIMER_SCHED, &timer);
    __assert(tmr_status == TMRR_SUCCESS);

    timer.oneshot_us(DEFAULT_TIMESLICE_USEC);
}

/*
 * Push a thread into the thread ready queue
 * allowing it to be eventually dequeued
 * and ran.
 */
static void
sched_enqueue_td(struct proc *td)
{
    /* Sanity check */
    if (td == NULL)
        return;

    spinlock_acquire(&tdq_lock);

    td->pid = nthread++;
    TAILQ_INSERT_TAIL(&td_queue, td, link);

    spinlock_release(&tdq_lock);
}

/*
 * Dequeue the first thread in the thread ready
 * queue.
 */
static struct proc *
sched_dequeue_td(void)
{
    struct proc *td = NULL;

    spinlock_acquire(&tdq_lock);

    if (TAILQ_EMPTY(&td_queue)) {
        goto done;
    }

    td = TAILQ_FIRST(&td_queue);
    TAILQ_REMOVE(&td_queue, td, link);
done:
    spinlock_release(&tdq_lock);
    return td;
}


/*
 * Processor awaiting tasks to be assigned will be here spinning.
 */
__noreturn static void
sched_enter(void)
{
    sched_oneshot();
    for (;;) {
        hint_spinwait();
    }
}

static struct proc *
sched_create_td(uintptr_t rip)
{
    const size_t STACK_SIZE = 0x100000; /* 1 MiB */
    struct proc *td;
    void *stack;
    struct trapframe *tf;

    tf = dynalloc(sizeof(struct trapframe));
    if (tf == NULL) {
        return NULL;
    }

    stack = dynalloc(STACK_SIZE);
    if (stack == NULL) {
        dynfree(tf);
        return NULL;
    }

    td = dynalloc(sizeof(struct proc));
    if (td == NULL) {
        dynfree(tf);
        dynfree(stack);
        return NULL;
    }

    memset(tf, 0, sizeof(struct trapframe));
    memset(stack, 0, STACK_SIZE);

    /* Setup process itself */
    td->pid = 0;        /* Don't assign PID until enqueued */
    td->cpu = NULL;     /* Not yet assigned a core */
    td->tf = tf;

    /* Setup trapframe */
    init_frame(tf, rip, (uintptr_t)stack + STACK_SIZE - 1);
    return td;
}

/*
 * Thread context switch routine
 */
void
sched_context_switch(struct trapframe *tf)
{
    struct cpu_info *ci = this_cpu();
    struct sched_state *state = &ci->sched_state;
    struct proc *td, *next_td;

    /*
     * If we have no threads, we should not
     * preempt at all.
     */
    if (nthread == 0) {
        goto done;
    } else if ((next_td = sched_dequeue_td()) == NULL) {
        /* Empty */
        goto done;
    }

    if (state->td != NULL) {
        /* Save our trapframe */
        td = state->td;
        memcpy(td->tf, tf, sizeof(struct trapframe));
    }

    /* Copy to stack */
    memcpy(tf, next_td->tf, sizeof(struct trapframe));

    td = state->td;
    state->td = next_td;

    if (td != NULL) {
        sched_enqueue_td(td);
    }
done:
    sched_oneshot();
}

void
sched_init(void)
{
    TAILQ_INIT(&td_queue);

    /*
     * TODO: Create init with sched_create_td()
     *       and enqueue with sched_enqueue_td()
     */
    (void)sched_create_td;
    (void)sched_enqueue_td;
}

/*
 * Setup scheduler related things and enqueue AP.
 */
void
sched_init_processor(struct cpu_info *ci)
{
    struct sched_state *sched_state = &ci->sched_state;
    (void)sched_state;      /* TODO */

    sched_enter();

    __builtin_unreachable();
}

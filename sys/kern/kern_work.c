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
#include <sys/errno.h>
#include <sys/panic.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/syslog.h>
#include <sys/workqueue.h>
#include <vm/dynalloc.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("workq: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

extern struct proc g_proc0;

/*
 * The workqueue cookie value that is used for
 * verifying if a workqueue object is properly
 * set up or not.
 */
#define WQ_COOKIE 0xFC0B

/*
 * A worker services work in the queue
 * and there is one per workqueue.
 */
static void
workqueue_worker(void)
{
    struct proc *td;
    struct workqueue *wqp;
    struct work *wp;

    td = this_td();
    if ((wqp = td->data) == NULL) {
        panic("no workqueue in thread\n");
    }

    /*
     * Weird things can happen, just be careful
     * here...
     */
    if (wqp->cookie != WQ_COOKIE) {
        panic("bad WQ_COOKIE in worker\n");
    }

    for (;;) {
        mutex_acquire(wqp->lock, 0);
        wp = TAILQ_FIRST(&wqp->work);

        /* Try again later if empty */
        if (wp == NULL) {
            mutex_release(wqp->lock);
            sched_yield();
            continue;
        }

        wp->func(wqp, wp);
        TAILQ_REMOVE(&wqp->work, wp, link);

        /*
         * Decrement the amount of work that is
         * left to get done. Check for underflows
         * which should not happen unless something
         * clobbers the fields.
         */
        if ((--wqp->nwork) < 0) {
            panic("wqp nwork underflow\n");
        }

        mutex_release(wqp->lock);
        sched_yield();
    }
}

/*
 * Allocates a new work queue that may be used
 * to hold queued up tasks.
 *
 * @name: Name to give the workqueue
 * @func: Function for work thread of this queue
 * @max_work: Maximum number of jobs to be added
 * @ipl: IPL that the work must operate in
 *
 * Returns a pointer to the new workqueue on success,
 * otherwise a value of NULL is returned.
 */
struct workqueue *
workqueue_new(const char *name, workfunc_t func, size_t max_work, int ipl)
{
    struct workqueue *wqp;
    struct proc *td;

    td = this_td();
    if (__unlikely(td == NULL)) {
        pr_error("no thread in workqueue_new()\n");
        return NULL;
    }

    wqp = dynalloc(sizeof(*wqp));
    if (wqp == NULL) {
        return NULL;
    }

    wqp->name = strdup(name);
    TAILQ_INIT(&wqp->work);
    wqp->ipl = ipl;
    wqp->max_work = max_work;
    wqp->nwork = 0;
    wqp->cookie = WQ_COOKIE;
    wqp->lock = mutex_new(wqp->name);
    wqp->func = func;

    /*
     * We need to spawn the work thread which
     * is behind the management of this specific
     * workqueue. It typically does something like
     * dequeuing at the head of the workqueue, performing
     * the work, cleaning up as needed and dequeuing the
     * next and waiting if there are none yet.
     */
    spawn(
        &g_proc0, workqueue_worker,
        wqp, 0,
        &wqp->worktd
    );

    return wqp;
}

/*
 * Enqueue a work item onto a specific
 * workqueue.
 *
 * @wqp: Pointer to specific workqueue
 * @wp: Pointer to work that should be enqueued
 *
 * Returns zero on success, otherwise a less than
 * zero value is returned.
 */
int
workqueue_enq(struct workqueue *wqp, struct work *wp)
{
    if (wqp == NULL || wp == NULL) {
        return -EINVAL;
    }

    /* Verify that we have a valid workqueue */
    if (__unlikely(wqp->cookie != WQ_COOKIE)) {
        panic("workq: bad cookie on work enqueue\n");
    }

    mutex_acquire(wqp->lock, 0);

    /*
     * If we have reached the max amount of jobs
     * that we can enqueue here, just log it and
     * bail.
     */
    if (wqp->nwork >= wqp->max_work) {
        pr_error("max jobs reached for '%s'\n", wqp->name);
        mutex_release(wqp->lock);
        return -EAGAIN;
    }

    TAILQ_INSERT_TAIL(&wqp->work, wp, link);
    ++wqp->nwork;
    mutex_release(wqp->lock);
    return 0;
}

/*
 * Destroy a workqueue and free resources
 * associated with it.
 *
 * @wqp: Pointer to workqueue to destroy
 *
 * Returns zero on success, otherwise a less
 * than zero value is returned.
 */
int
workqueue_destroy(struct workqueue *wqp)
{
    if (wqp == NULL) {
        return -EINVAL;
    }

    /* Should not happen but just make sure */
    if (__unlikely(wqp->cookie != WQ_COOKIE)) {
        panic("workq: bad cookie on destroy\n");
    }

    /* Free the name if we have it */
    if (wqp->name != NULL) {
        dynfree(wqp->name);
    }

    if (wqp->lock != NULL) {
        mutex_free(wqp->lock);
    }

    /* Brutally murder any workthreads */
    if (wqp->worktd != NULL) {
        exit1(wqp->worktd, 0);
        wqp->worktd = NULL;
    }

    /*
     * Zero before we free for security reasons, we
     * don't really know what will be queued up but
     * for certain things, it is best if we make it
     * as if it never existed in the first place.
     *
     * XXX: There is no need to free the workqueue here as
     *      we had to pass it to spawn() to run the worker.
     *
     *      During an exit, spawn() will free the thread data
     *      meaning this is already cleaned up.
     */
    memset(wqp, 0, sizeof(*wqp));
    return 0;
}

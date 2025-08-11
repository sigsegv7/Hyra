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

#ifndef _SYS_WORKQUEUE_H_
#define _SYS_WORKQUEUE_H_

#if defined(_KERNEL)

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/proc.h>

struct workqueue;
struct work;

/*
 * A work function can either refer to a work thread
 * entry (or actual work to be done
 */
typedef void(*workfunc_t)(struct workqueue *wqp, struct work *wp);

/*
 * Represents work that may be added to a
 * workqueue.
 *
 * @name: Name of this work/task [i]
 * @data: Optional data to be passed with work [p]
 * @func: Function with work to be done [p]
 * @cookie: Used for validating the work structure [i]
 *
 * Field attributes:
 * - [i]: Used internally
 * - [p]: Used as parameter
 */
struct work {
    char *name;
    void *data;
    workfunc_t func;
    TAILQ_ENTRY(work) link;
};

/*
 * A workqueue contains tasks that are
 * queued up to be completed in their own
 * thread context.
 *
 * @name: Name of workqueue.
 * @work: Start of the workqueue
 * @ipl: IPL that work here must run with
 * @max_work: Max number of jobs that can be queued
 * @nwork: Number of tasks to be done
 * @cookie: For validating workqueues
 * @worktd: Thread associated with the workqueue
 * @lock: Protects the workqueue
 */
struct workqueue {
    char *name;
    TAILQ_HEAD(, work) work;
    uint8_t ipl;
    size_t max_work;
    ssize_t nwork;
    uint16_t cookie;
    struct proc *worktd;
    struct mutex *lock;
};

struct workqueue *workqueue_new(const char *name, size_t max_work, int ipl);

int workqueue_enq(struct workqueue *wqp, const char *name, struct work *wp);
int workqueue_destroy(struct workqueue *wqp);
int work_destroy(struct work *wp);

#endif  /* !_KERNEL */
#endif  /* !_SYS_WORKQUEUE_H_ */

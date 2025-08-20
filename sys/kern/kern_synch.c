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
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/sched.h>
#include <sys/atomic.h>
#include <sys/syslog.h>
#include <sys/spinlock.h>
#include <machine/cdefs.h>
#include <dev/timer.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("synch: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

/*
 * Returns 0 on success, returns non-zero value
 * on timeout/failure.
 */
int
spinlock_usleep(struct spinlock *lock, size_t usec_max)
{
    struct timer tmr;
    size_t usec_start, usec_cur;
    size_t usec_elap;
    int err;

    if ((err = req_timer(TIMER_GP, &tmr)) != 0) {
        pr_error("spinlock_usleep: req_timer() returned %d\n", err);
        pr_error("spinlock_usleep: Failed to request timer...\n");
        return err;
    }

    if (tmr.get_time_usec == NULL) {
        pr_error("spinlock_usleep: Timer does not have get_time_usec()\n");
        return -ENOTSUP;
    }

    usec_start = tmr.get_time_usec();
    while (__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE)) {
        usec_cur = tmr.get_time_usec();
        usec_elap = (usec_cur - usec_start);

        if (usec_elap > usec_max) {
            return -1;
        }
    }

    return 0;
}

void
spinlock_acquire(struct spinlock *lock)
{
    sched_preempt_set(false);
    while (__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE)) {
        md_pause();
    }
}

/*
 * Lazy acquire a spinlock
 *
 * spinlock_try_acquire() may only spin one thread
 * at a time, threads that want to spin too must
 * explicity do it on their own.
 *
 * This function returns 1 (a value that may be
 * spinned on) when the lock is acquired and a
 * thread is already spinning on it.
 */
int
spinlock_try_acquire(struct spinlock *lock)
{
    volatile int locked;

    locked = atomic_load_int(&lock->lock);
    if (locked != 0) {
        return 1;
    }

    return __atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE);
}

void
spinlock_release(struct spinlock *lock)
{
    __atomic_clear(&lock->lock, __ATOMIC_RELEASE);
    sched_preempt_set(true);
}

/*
 * Create a new mutex lock object
 */
struct mutex *
mutex_new(const char *name)
{
    struct mutex *mtx;
    size_t namelen;

    mtx = dynalloc(sizeof(*mtx));
    if (mtx == NULL) {
        return NULL;
    }

    mtx->lock = 0;
    namelen = strlen(name);

    /* Don't overflow the name buffer */
    if (namelen >= MUTEX_NAME_LEN) {
        namelen = MUTEX_NAME_LEN - 1;
    }

    memcpy(mtx->name, name, namelen);
    return mtx;
}

/*
 * Acquire a mutex
 *
 * @mtx: Mutex to acquire
 * @flags: Optional flags
 */
int
mutex_acquire(struct mutex *mtx, int flags)
{
    while (__atomic_test_and_set(&mtx->lock, __ATOMIC_ACQUIRE)) {
        sched_yield();
    }

    return 0;
}

void
mutex_release(struct mutex *mtx)
{
    __atomic_clear(&mtx->lock, __ATOMIC_RELEASE);
}

void
mutex_free(struct mutex *mtx)
{
    dynfree(mtx);
}

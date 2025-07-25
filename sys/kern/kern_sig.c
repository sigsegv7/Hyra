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

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/param.h>
#include <vm/dynalloc.h>
#include <string.h>

#define sigmask(SIGNO) BIT(SIGNO)

static struct sigaction sa_tab[] = {
    [SIGFPE] = {
        .sa_handler = sigfpe_default,
        .sa_mask = 0,
        .sa_flags = 0,
        .sa_sigaction = NULL
    },

    [SIGKILL] = {
        .sa_handler = sigkill_default,
        .sa_mask = 0,
        .sa_flags = 0,
        .sa_sigaction = NULL
    },

    [SIGSEGV] = {
        .sa_handler = sigsegv_default,
        .sa_mask = 0,
        .sa_flags = 0,
        .sa_sigaction = NULL
    },
    [SIGTERM] = {
        .sa_handler = sigterm_default,
        .sa_mask = 0,
        .sa_flags = 0,
        .sa_sigaction = NULL
    }
};

/*
 * Register a new signal descriptor, set it in the process
 * structure and return it in `ksig`.
 *
 * @td: Process to register signal to.
 * @signo: Signal number to register.
 * @ksig: Will contain a pointer to new signal descriptor.
 */
int
newsig(struct proc *td, int signo, struct ksiginfo **ksig)
{
    struct ksiginfo *ksig_tmp;

    /* Ensure we have valid args */
    if (td == NULL || signo >= PROC_SIGMAX)
        return -EINVAL;

    /*
     * If we already have a signal registered in a slot, free up
     * memory used for that signal descriptor so we can override
     * it with the new one.
     */
    if ((ksig_tmp = td->ksig_list[signo]) != NULL)
        dynfree(ksig_tmp);

    /* Allocate our new signal */
    ksig_tmp = dynalloc(sizeof(*ksig_tmp));
    if (ksig_tmp == NULL)
        return -ENOMEM;

    memset(ksig_tmp, 0, sizeof(*ksig_tmp));
    ksig_tmp->signo = signo;
    td->ksig_list[signo] = ksig_tmp;
    *ksig = ksig_tmp;
    return 0;
}

/*
 * Remove a signal from the signal table.
 *
 * @td: Process to remove signal from.
 * @signo: Signal to remove.
 */
int
delsig(struct proc *td, int signo)
{
    struct ksiginfo *ksig_tmp;

    /* Ensure we have valid args */
    if (td == NULL || signo >= PROC_SIGMAX)
        return -EINVAL;

    /* Don't do anything if it doesn't exist */
    if ((ksig_tmp = td->ksig_list[signo]) == NULL)
        return 0;

    dynfree(ksig_tmp);
    td->ksig_list[signo] = NULL;
    return 0;
}

int
sendsig(struct proc *td, const sigset_t *set)
{
    struct ksiginfo *ksig_tmp;

    /* Ensure arguments are correct */
    if (td == NULL || set == NULL)
        return -EINVAL;

    /* Enqueue required ksiginfo structures */
    for (int i = 0; i < PROC_SIGMAX; ++i) {
        if ((ksig_tmp = td->ksig_list[i]) == NULL) {
            continue;
        }

        /* Enqueue if it is a member of the sigset */
        if (sigismember(set, i)) {
            spinlock_acquire(&td->ksigq_lock);
            TAILQ_INSERT_TAIL(&td->ksigq, ksig_tmp, link);
            spinlock_release(&td->ksigq_lock);
        }
    }

    return 0;
}

int
signals_init(struct proc *td)
{
    struct sigaction *sa;
    struct ksiginfo *ksig;
    int error, i;

    TAILQ_INIT(&td->ksigq);

    /* Populate process signal table with defaults */
    for (i = 0; i < NELEM(sa_tab); ++i) {
        sa = &sa_tab[i];

        /* Drop actions that aren't set up */
        if (sa->sa_handler == NULL && sa->sa_sigaction == NULL) {
            continue;
        }

        /* Attempt to register the new signal */
        if ((error = newsig(td, i, &ksig)) != 0) {
            goto fail;
        }

        ksig->si = sa;
    }

    return 0;

fail:
    /*
     * We do not want the process signal table to be in
     * an inconsistent state so we clean up on failure.
     */
    for (int j = 0; j < i; ++j) {
        if ((error = delsig(td, j)) != 0) {
            kprintf("delsig() failed on signal %d (returned %d)\n",
                j, error);
        }
    }

    return error;
}

void
dispatch_signals(struct proc *td)
{
    struct ksiginfo *ksig_tmp;
    struct sigaction *action;

    spinlock_acquire(&td->ksigq_lock);
    while (!TAILQ_EMPTY(&td->ksigq)) {
        /* Dequeue signal descriptor */
        ksig_tmp = TAILQ_FIRST(&td->ksigq);
        TAILQ_REMOVE(&td->ksigq, ksig_tmp, link);

        /* Invoke handler */
        action = ksig_tmp->si;
        action->sa_handler(ksig_tmp->signo);
    }

    spinlock_release(&td->ksigq_lock);
}

int
sigemptyset(sigset_t *set)
{
    *set = 0;
    return 0;
}

int
sigfillset(sigset_t *set)
{
    *set = ~(sigset_t)0;
    return 0;
}

int
sigaddset(sigset_t *set, int signo)
{
    if (signo <= 0 || signo >= PROC_SIGMAX)
        return -EINVAL;

    *set |= sigmask(signo);
    return 0;
}

int
sigdelset(sigset_t *set, int signo)
{
    if (signo <= 0 || signo >= PROC_SIGMAX)
        return -EINVAL;

    *set &= ~sigmask(signo);
    return 0;
}

int
sigismember(const sigset_t *set, int signo)
{
    if (signo <= 0 || signo >= PROC_SIGMAX)
        return -EINVAL;

    return (*set & sigmask(signo)) != 0;
}

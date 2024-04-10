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

#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <sys/signal.h>
#include <dev/vcons/vcons.h>
#include <string.h>

__MODULE_NAME("kern_signal");
__KERNEL_META("$Hyra$: kern_signal.c, Ian Marco Moffett, "
              "Signal handling code");

static void
signal_log(const char *s)
{
    vcons_putstr(&g_syslog_screen, s, strlen(s));
}

/*
 * Handle any signals within the current thread
 *
 * TODO: Add sigaction support, default action
 *       for all currently is killing the process.
 */
void
signal_handle(struct proc *curtd)
{
    int signo = curtd->signal;

    spinlock_acquire(&curtd->lock);
    if (signo == 0) {
        return;
    }

    curtd->signal = 0;
    switch (signo) {
    case SIGFPE:
        signal_log("Arithmetic error\n");
        break;
    case SIGSEGV:
        signal_log("Segmentation fault\n");
        break;
    case SIGKILL:
        signal_log("Killed\n");
        break;
    }

    spinlock_release(&curtd->lock);
    sched_exit();
}

/*
 * Raise a signal for a process
 *
 * @to: Can be NULL to mean the current process
 * @signal: Signal to send
 *
 * TODO: Add more functionality.
 */
void
signal_raise(struct proc *to, int signal)
{
    if (to == NULL) {
        to = this_td();
    }

    to->signal = signal;
    if (to == this_td()) {
        /* Current process, just preempt */
        sched_context_switch(to->tf);
    }
}

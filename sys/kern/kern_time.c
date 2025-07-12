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
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <dev/timer.h>
#include <machine/cdefs.h>

/*
 * arg0: Timespec
 * arg1: Remaining timeval
 */
scret_t
sys_sleep(struct syscall_args *scargs)
{
    struct timespec ts;
    struct timer tmr;
    size_t timeout_msec;
    tmrr_status_t status;
    int error;

    error = copyin((void *)scargs->arg0, &ts, sizeof(ts));
    if (error < 0) {
        return error;
    }

    if (ts.tv_nsec >= 1000000000) {
        return -EINVAL;
    }

    status = req_timer(TIMER_GP, &tmr);
    if (__unlikely(status != TMRR_SUCCESS)) {
        return -ENOTSUP;
    }
    if (__unlikely(tmr.msleep == NULL)) {
        return -ENOTSUP;
    }

    timeout_msec = ts.tv_nsec / 1000000;
    timeout_msec += ts.tv_sec * 1000;
    tmr.msleep(timeout_msec);
    return 0;
}

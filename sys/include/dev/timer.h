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

#ifndef _DEV_TIMER_H_
#define _DEV_TIMER_H_

#include <sys/types.h>

/* Timer IDs */
#define TIMER_SCHED 0x00000000U     /* Scheduler reserved timer */
#define TIMER_GP    0x00000001U     /* General purpose timer */

/* Number of timer IDs, adjust when adding timer IDs */
#define TIMER_ID_COUNT 2

/* Timer registry status */
#define TMRR_SUCCESS            0x00000000U     /* Successful */
#define TMRR_HAS_ENTRY          0x00000001U     /* Already has an entry */
#define TMRR_INVALID_TYPE       0x00000002U     /* Invalid timer id */
#define TMRR_EMPTY_ENTRY        0x00000003U     /* Entry is empty */
#define TMRR_INVALID_ARG        0x00000004U     /* Invalid iface arg */

/* See timer ID defines */
typedef uint8_t timer_id_t;

/* See timer registry status */
typedef int tmrr_status_t;

/*
 * Represents a timer, pointer fields
 * are optional and may be set to NULL, therefore
 * it is paramount to verify any function or general
 * pointer field within this struct is checked for
 * a NULL value. Fields should be NULL if the timer
 * driver implementation doesn't implement support
 * for a functionality.
 *
 * XXX: The msleep, usleep, ... functions must return
 *      either EXIT_SUCCESS and EXIT_FAILURE from sys/errno.h
 *      ONLY.
 */
struct timer {
    const char *name;               /* e.g "HPET" */
    size_t(*calibrate)(void);       /* Returns frequency, 0 for unspecified */
    size_t(*get_time_usec)(void);   /* Time since init (microseconds) */
    size_t(*get_time_nsec)(void);   /* Time since init (nanoseconds) */
    size_t(*get_time_sec)(void);    /* Time since init (seconds) */
    int(*msleep)(size_t ms);
    int(*usleep)(size_t us);
    int(*nsleep)(size_t ns);
    void(*periodic_ms)(size_t ms);
    void(*periodic_us)(size_t ms);
    void(*oneshot_ms)(size_t ms);
    void(*oneshot_us)(size_t ms);
    void(*stop)(void);
};

tmrr_status_t register_timer(timer_id_t id, const struct timer *tmr);
tmrr_status_t tmr_registry_overwrite(timer_id_t, const struct timer *tmr);
tmrr_status_t req_timer(timer_id_t id, struct timer *tmr_out);

#endif  /* !_DEV_TIMER_H_ */

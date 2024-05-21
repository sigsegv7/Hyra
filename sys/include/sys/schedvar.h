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

#ifndef _SYS_SCHEDVAR_H_
#define _SYS_SCHEDVAR_H_

#include <sys/cdefs.h>
#include <sys/proc.h>
#include <sys/queue.h>

#define DEFAULT_TIMESLICE_USEC 3000
#define SHORT_TIMESLICE_USEC 10

#define SCHED_POLICY_MLFQ 0x0000U   /* Multilevel feedback queue */
#define SCHED_POLICY_RR   0x0001U   /* Round robin */

typedef uint8_t schedpolicy_t;

/* Might be set by kconf(1) */
#if defined(__SCHED_NQUEUE)
#define SCHED_NQUEUE __SCHED_NQUEUE
#else
#define SCHED_NQUEUE 4
#endif

/* Ensure SCHED_NQUEUE is an acceptable value */
__STATIC_ASSERT(SCHED_NQUEUE <= 8, "SCHED_NQUEUE exceeds max");
__STATIC_ASSERT(SCHED_NQUEUE > 0, "SCHED_NQUEUE cannot be zero");

struct sched_queue {
    TAILQ_HEAD(, proc) q;
    size_t nthread;
};

#endif /* !_SYS_SCHEDVAR_H_ */

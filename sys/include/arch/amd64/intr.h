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

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_

#include <sys/types.h>

#define IST_SCHED   1U
#define IST_HW_IRQ  2U
#define IST_SW_INT  3U
#define IST_SYSCALL 4U

/* Upper 4 bits of interrupt vector */
#define IPL_SHIFT 4

/*
 * Interrupt priority levels
 */
#define IPL_NONE    0   /* Don't defer anything */
#define IPL_BIO     1   /* Block I/O */
#define IPL_CLOCK   2   /* Clock */
#define IPL_HIGH    3   /* Defer everything */

#define N_IPIVEC 4      /* Number of vectors reserved for IPIs */
#define IPI_PER_VEC 16  /* Max IPIs per vector */

struct intr_hand;

/*
 * Contains information passed to driver
 *
 * @ihp: Interrupt handler
 * @data: Driver specific data
 */
struct intr_data {
    struct intr_hand *ihp;
    union {
        void *data;
        uint64_t data_u64;
    };
};

/*
 * Interrupt handler
 *
 * [r]: Required for intr_register()
 * [o]: Not required for intr_register()
 * [v]: Returned by intr_register()
 * [i]: Internal
 *
 * @func: The actual handler        [r]
 * @data: Interrupt data            [o/v]
 * @nintr: Number of times it fired [o]
 * @name: Interrupt name            [v]
 * @priority: Interrupt priority    [r]
 * @irq: Interrupt request number   [o]
 * @vector: Interrupt vector        [v]
 *
 * XXX: `name' must be null terminated ('\0')
 *
 * XXX: `irq` can be set to -1 for MSI/MSI-X
 *      interrupts.
 *
 * XXX: `func` must be the first field in this
 *      structure so that it may be called through
 *      assembly.
 *
 * XXX: `ist' should usually be set to -1 but can be
 *      used if an interrupt requires its own stack.
 */
struct intr_hand {
    int(*func)(void *);
    size_t nintr;
    struct intr_data data;
    char *name;
    int priority;
    int irq;
    int vector;
};

void *intr_register(const char *name, const struct intr_hand *ih);

int splraise(uint8_t s);
void splx(uint8_t s);

#endif

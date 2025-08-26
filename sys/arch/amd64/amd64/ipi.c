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
#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/panic.h>
#include <sys/spinlock.h>
#include <machine/cpu.h>
#include <machine/idt.h>
#include <machine/ipi.h>
#include <machine/lapic.h>
#include <vm/dynalloc.h>
#include <string.h>

void ipi_isr(void);
void halt_isr(void);

void __ipi_handle_common(void);

#define pr_trace(fmt, ...) kprintf("ipi: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define COOKIE 0x7E0A
#define MAX_IPI 32

/* For the global state of the subsystem */
static uint32_t cookie = 0;

static struct cpu_ipi ipi_list[MAX_IPI];
static uint8_t ipi_count = 0;
static struct spinlock lock;

/*
 * Allocate an IPI that can be sent to other
 * cores on the CPU. This is the core logic
 * and contains *no* locks. One should be
 * using the md_ipi_alloc() function instead.
 *
 * Returns the allocated IPI identifier on succes,
 * otherwise a less than zero value is returned.
 */
static int
__ipi_alloc(struct cpu_ipi **res)
{
    struct cpu_ipi *ipip;

    if (ipi_count >= MAX_IPI) {
        return -EAGAIN;
    }

    ipip = &ipi_list[ipi_count];
    ipip->cookie = COOKIE;
    ipip->id = ipi_count++;
    ipip->handler = NULL;
    *res = ipip;
    return ipip->id;
}

/*
 * Common IPI routine, called from vector.S
 *
 * XXX: Internal usage only
 */
void
__ipi_handle_common(void)
{
    struct cpu_ipi *ipip;
    struct cpu_info *ci = this_cpu();
    ipi_pend_t pending = 0;

    if (cookie != COOKIE) {
        pr_trace("[warn]: got spurious ipi\n");
        return;
    }

    if (ci == NULL) {
        pr_error("could not get current CPU\n");
        return;
    }

    if (ipi_count == 0) {
        pr_error("no registered IPIs\n");
        return;
    }

    /* Attempt to find a handler */
    pending = ci->ipi_pending;
    for (int i = 0; i < ipi_count; ++i) {
        ipip = &ipi_list[i];
        if (ISSET(pending, BIT(i))) {
            ipip->handler(ipip);
            ci->ipi_pending &= ~BIT(i);
        }
    }

    /* We are done dispatching IPIs */
    ci->ipi_dispatch = 0;
}

/*
 * Send one or more IPIs to a specific
 * processor after caller sets bits in
 * the `ci->ipi_pending' field
 *
 * @ci: Processor to send IPI(s) to
 * @ipi: IPIs to send
 */
int
md_ipi_send(struct cpu_info *ci, ipi_pend_t ipi)
{
    uint32_t apic_id = 0;

    if (ci != NULL) {
        /*
         * We are already dispatching IPIs, we don't
         * want to find ourselves in interrupt hell.
         */
        if (ci->ipi_dispatch) {
            return -EAGAIN;
        }

        apic_id = ci->apicid;
    }

    ci->ipi_dispatch = 1;
    ci->ipi_pending |= BIT(ipi);

    /* Send it through on the bus */
    lapic_send_ipi(
        apic_id,
        IPI_SHORTHAND_NONE,
        IPI_VECTOR
    );
    return 0;
}

/*
 * IPI allocation interface with
 * locking.
 */
int
md_ipi_alloc(struct cpu_ipi **res)
{
    int retval;

    spinlock_acquire(&lock);
    retval = __ipi_alloc(res);
    spinlock_release(&lock);
    return retval;
}

/*
 * Initialize the IPI thunks
 */
void
md_ipi_init(void)
{
    /* Initialize the IPI vectors */
    idt_set_desc(IPI_VECTOR, IDT_INT_GATE, ISR(ipi_isr), 0);
    idt_set_desc(HALT_VECTOR, IDT_INT_GATE, ISR(halt_isr), 0);
    cookie = COOKIE;
}

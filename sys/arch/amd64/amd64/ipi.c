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
#include <string.h>

void ipi_isr0(void);
void ipi_isr1(void);
void ipi_isr2(void);
void ipi_isr3(void);

void __ipi_handle_common(void);

#define pr_trace(fmt, ...) kprintf("ipi: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define BASE_VECTOR 0x21
#define COOKIE 0x7E0A

/* For the global state of the subsystem */
static uint32_t cookie = 0;

/*
 * The next vector that will be used for an IPI to
 * be allocated. It starts at 0x21 because interrupt
 * vector 0x20 is used for the Hyra scheduler and `N_IPIVEC'
 * vectors up are reserved for inter-processor interrupts.
 *
 * XXX: This must not go beyond N_IPIVEC !!
 */
static uint8_t next_vec = BASE_VECTOR;
static uint8_t vec_entries = 0;

/*
 * In order to get an index into the 'vectors' array,
 * one can pass an `ipi_bitmap' bit index into the
 * ipi_vector() function. The index into the `ipi`
 * field within may be acquired with the ipi_index()
 * function.
 */
static uint64_t ipi_bitmap = 0;
static struct ipi_vector vectors[N_IPIVEC];
static struct spinlock lock;

/*
 * Allocate a bit from the `ipi_bitmap' and
 * return the index.
 *
 * Returns a less than zero value upon error.
 */
static ssize_t
alloc_ipi_bit(void)
{
    const size_t MAX = sizeof(ipi_bitmap) * 8;
    off_t i;

    for (i = 0; i < MAX; ++i) {
        if (!ISSET(ipi_bitmap, BIT(i))) {
            ipi_bitmap |= BIT(i);
            return i;
        }
    }

    return -1;
}

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
    struct ipi_vector *vp;
    struct cpu_ipi *ipip;
    ssize_t bit;
    uint8_t idx;

    if (res == NULL) {
        return -EINVAL;
    }

    if (next_vec >= BASE_VECTOR + N_IPIVEC) {
        return -EAGAIN;
    }

    /*
     * Attempt to allocate a bit index from
     * the bitmap.
     */
    if ((bit = alloc_ipi_bit()) < 0) {
        return -EAGAIN;
    }

    idx = ipi_vector(bit);
    vp = &vectors[idx];

    /* Initialize the vector if not already */
    if (vp->cookie != COOKIE) {
        vp->cookie = COOKIE;
        vp->nipi = 0;
        vp->vec = next_vec;
        memset(vp->ipi, 0, sizeof(vp->ipi));
    }

    /*
     * Just a sanity check here, the number of
     * IPIs per vector should never exceed the
     * maximum, and if it does, that gives us more
     * than enough grounds to panic the system as
     * it would not be wise to trust it.
     */
    if (__unlikely(vp->nipi >= IPI_PER_VEC)) {
        panic("too many IPIs in vector %x\n", vp->vec);
    }

    idx = ipi_index(bit);
    ipip = &vp->ipi[idx];

    /* We are allocating, not clobbering */
    if (ipip->cookie == COOKIE) {
        panic("ipi table corruption\n");
    }

    if ((++vec_entries) >= IPI_PER_VEC) {
        vec_entries = 0;
        ++next_vec;
    }

    /* Set up the initial state */
    ipip->cookie = COOKIE;
    ipip->handler = NULL;
    ipip->id = bit;
    *res = ipip;
    return bit;
}

/*
 * Dispatch pending IPIs for the current
 * processor.
 *
 * @vector: Backing interrupt vector
 * @ci: Current processor
 */
static void
ipi_dispatch_pending(struct ipi_vector *vec, struct cpu_info *ci)
{
    uint8_t bit_i;
    uint8_t n_bit;
    uint8_t index;
    struct cpu_ipi *ipip = NULL;
    ipi_pend_t pending;

    if (vec == NULL || ci == NULL) {
        return;
    }

    n_bit = sizeof(pending) * 8;
    for (bit_i = 0; bit_i < n_bit; ++bit_i) {
        index = ipi_vector(bit_i);
        pending = ci->ipi_pending[index];

        vec = &vectors[index];
        index = ipi_index(bit_i);
        ipip = &vec->ipi[index];

        /* Is this pending or not? */
        if (!ISSET(pending, BIT(bit_i))) {
            continue;
        }

        /* Handle and mark as no longer pending */
        ipip->handler(ipip);
        ci->ipi_pending[vec->vec] &= ~BIT(bit_i);
    }
}

/*
 * Check an IPI pending bitmap for a
 * vector and send IPIs as needed
 *
 * @ci: Target processor
 * @pending: Pending IPIs
 */
static void
ipi_send_vector(struct cpu_info *ci, ipi_pend_t pending)
{
    struct ipi_vector *vp;
    struct cpu_ipi *ipip;
    uint8_t n_bits = sizeof(pending) * 8;
    uint8_t bit_i;
    uint8_t vector, index;
    uint32_t apic_id = 0;

    if (ci != NULL) {
        /*
         * We are already dispatching IPIs, we don't
         * want to find ourselves in interrupt hell.
         */
        if (ci->ipi_dispatch) {
            return;
        }

        apic_id = ci->apicid;
    }

    ci->ipi_dispatch = 1;
    for (bit_i = 0; bit_i < n_bits; ++bit_i) {
        if (ISSET(pending, BIT(bit_i))) {
            vector = ipi_vector(bit_i);
            index = ipi_index(bit_i);

            if (ci != NULL)
                ci->ipi_id = bit_i;

            vp = &vectors[vector];
            ipip = &vp->ipi[index];

            /* Ignore if cookie does match */
            if (ipip->cookie != COOKIE)
                continue;

            /* Ignore if there is no handler */
            if (ipip->handler == NULL)
                continue;

            /* Send that IPI through */
            lapic_send_ipi(
                apic_id,
                IPI_SHORTHAND_NONE,
                BASE_VECTOR + vector
            );
        }
    }
}

/*
 * Common IPI routine, called from vector.S
 *
 * XXX: Internal usage only
 */
void
__ipi_handle_common(void)
{
    struct ipi_vector *vp;
    struct cpu_info *ci = this_cpu();
    uint8_t vector;

    if (cookie != COOKIE) {
        pr_trace("[warn]: got spurious ipi\n");
        return;
    }

    /* Grab the vector */
    vector = ipi_vector(ci->ipi_id);
    vp = &vectors[vector];
    if (vp->cookie != COOKIE) {
        pr_error("got IPI for uninitialized vector\n");
        return;
    }

    if ((ci = this_cpu()) == NULL) {
        pr_error("could not get current CPU\n");
        return;
    }

    ipi_dispatch_pending(vp, ci);

    /* We are done dispatching IPIs */
    ci->ipi_dispatch = 0;
    ci->ipi_id = 0;
}

/*
 * Send one or more IPIs to a specific
 * processor after caller sets bits in
 * the `ci->ipi_pending' field
 *
 * @ci: Processor to send IPI(s) to
 */
int
md_ipi_send(struct cpu_info *ci)
{
    if (ci == NULL) {
        return -EINVAL;
    }

    spinlock_acquire(&lock);
    for (int i = 0; i < N_IPIVEC; ++i) {
        ipi_send_vector(ci, ci->ipi_pending[i]);
    }

    spinlock_release(&lock);
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
    idt_set_desc(0x21, IDT_INT_GATE, ISR(ipi_isr0), 0);
    idt_set_desc(0x22, IDT_INT_GATE, ISR(ipi_isr1), 0);
    idt_set_desc(0x23, IDT_INT_GATE, ISR(ipi_isr2), 0);
    idt_set_desc(0x24, IDT_INT_GATE, ISR(ipi_isr3), 0);
    cookie = COOKIE;
}

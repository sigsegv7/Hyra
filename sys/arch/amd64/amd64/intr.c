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
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <machine/intr.h>
#include <machine/cpu.h>
#include <machine/asm.h>
#include <machine/ioapic.h>
#include <vm/dynalloc.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("intr: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

struct intr_hand *g_intrs[256] = {0};

int
splraise(uint8_t s)
{
    struct cpu_info *ci = this_cpu();
    int old_ipl;

    if (s < ci->ipl) {
        panic("splraise IPL less than current IPL\n");
    }

    amd64_write_cr8(s);
    old_ipl = ci->ipl;
    ci->ipl = s;
    return old_ipl;
}

void
splx(uint8_t s)
{
    struct cpu_info *ci = this_cpu();

    if (s > ci->ipl) {
        panic("splx IPL greater than current IPL\n");
    }

    amd64_write_cr8(s);
    ci->ipl = s;
}

void *
intr_register(const char *name, const struct intr_hand *ih)
{
    uint32_t vec = MAX(ih->priority << IPL_SHIFT, 0x20);
    struct intr_hand *ih_new;
    struct intr_data *idp_new;
    const struct intr_data *idp;
    size_t name_len;

    /* Sanity check */
    if (vec > NELEM(g_intrs) || name == NULL) {
        return NULL;
    }

    ih_new = dynalloc(sizeof(*ih_new));
    if (ih_new == NULL) {
        pr_error("could not allocate new interrupt handler\n");
        return NULL;
    }

    /*
     * Try to allocate an interrupt vector. An IPL is made up
     * of 4 bits so there can be 16 vectors per IPL.
     *
     * XXX: Vector 0x20 is reserved for the Hyra scheduler and
     *      vectors 0x21 to 0x21 + N_IPIVEC are reserved for
     *      inter-processor interrupts.
     */
    for (int i = vec; i < vec + 16; ++i) {
        if (g_intrs[i] != NULL || i < 0x24) {
            continue;
        }

        /* Allocate memory for the name */
        name_len = strlen(name) + 1;
        ih_new->name = dynalloc(name_len);
        if (ih_new->name == NULL) {
            dynfree(ih_new);
            pr_trace("could not allocate interrupt name\n");
            return NULL;
        }

        memcpy(ih_new->name, name, name_len);
        idp_new = &ih_new->data;
        idp = &ih->data;

        /* Pass the interrupt data */
        idp_new->ihp = ih_new;
        idp_new->data_u64 = idp->data_u64;

        /* Setup the new intr_hand */
        ih_new->func = ih->func;
        ih_new->priority = ih->priority;
        ih_new->irq = ih->irq;
        ih_new->vector = i;
        ih_new->nintr = 0;
        g_intrs[i] = ih_new;

        if (ih->irq >= 0) {
            ioapic_set_vec(ih->irq, i);
            ioapic_irq_unmask(ih->irq);
        }
        return ih_new;
    }

    return NULL;
}

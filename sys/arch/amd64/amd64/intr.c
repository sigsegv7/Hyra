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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <machine/intr.h>
#include <machine/cpu.h>
#include <machine/asm.h>
#include <vm/dynalloc.h>

static struct intr_entry *intrs[256] = {0};

void
splraise(uint8_t s)
{
    struct cpu_info *ci = this_cpu();

    if (s < ci->ipl) {
        panic("splraise IPL less than current IPL\n");
    }

    amd64_write_cr8(s);
    ci->ipl = s;
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

int
intr_alloc_vector(const char *name, uint8_t priority)
{
    size_t vec = MAX(priority << IPL_SHIFT, 0x20);
    struct intr_entry *intr;

    /* Sanity check */
    if (vec > NELEM(intrs)) {
        return -1;
    }

    /*
     * Try to allocate an interrupt vector. An IPL is made up
     * of 4 bits so there can be 16 vectors per IPL.
     */
    for (int i = vec; i < vec + 16; ++i) {
        if (intrs[i] != NULL) {
            continue;
        }

        intr = dynalloc(sizeof(*intr));
        if (intr == NULL) {
            return -ENOMEM;
        }

        intr->priority = priority;
        intrs[i] = intr;
        return i;
    }

    return -1;
}

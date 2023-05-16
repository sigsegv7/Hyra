/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

#include <arch/x86/idt.h>

#if defined(__x86_64__)

static struct idt_gate idt[256];
static struct idtr idtr = {
    .limit  = sizeof(struct idt_gate) * 256 - 1,
    .offset = (uintptr_t)&idt[0]
};

void
idt_load(void)
{
    __asm("lidt %0"
          :
          : "m" (idtr)
    );
}

void idt_set_desc(uint8_t vec, uint8_t type, uintptr_t isr,
                  uint8_t ist)
{
    if (vec >= 256) {
        return;
    }

    struct idt_gate *desc = &idt[vec];
    desc->offset_lo  = (uint16_t)isr;
    desc->offset_mid = (uint16_t)(isr >> 16);
    desc->offset_hi  = (uint32_t)(isr >> 32);
    desc->cs = 0x8;
    desc->type = type;
    desc->dpl = 3;
    desc->p = 1;
    desc->zero = 0;
    desc->zero1 = 0;
    desc->reserved = 0;
    desc->ist = ist;
}

#endif

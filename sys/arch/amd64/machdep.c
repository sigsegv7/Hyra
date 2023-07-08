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

/* $Id$ */

#include <sys/machdep.h>
#include <sys/cdefs.h>
#include <machine/trap.h>
#include <machine/idt.h>

#define ISR(func) ((uintptr_t)func)

__weak void
interrupts_init(struct processor *processor)
{
    __USE(processor);
    idt_set_desc(0x0, IDT_TRAP_GATE_FLAGS, ISR(arith_err), 0);
    idt_set_desc(0x2, IDT_TRAP_GATE_FLAGS, ISR(nmi), 0);
    idt_set_desc(0x3, IDT_TRAP_GATE_FLAGS, ISR(breakpoint_handler), 0);
    idt_set_desc(0x4, IDT_TRAP_GATE_FLAGS, ISR(overflow), 0);
    idt_set_desc(0x5, IDT_TRAP_GATE_FLAGS, ISR(bound_range), 0);
    idt_set_desc(0x6, IDT_TRAP_GATE_FLAGS, ISR(invl_op), 0);
    idt_set_desc(0x8, IDT_TRAP_GATE_FLAGS, ISR(double_fault), 0);
    idt_set_desc(0xA, IDT_TRAP_GATE_FLAGS, ISR(invl_tss), 0);
    idt_set_desc(0xB, IDT_TRAP_GATE_FLAGS, ISR(segnp), 0);
    idt_set_desc(0xD, IDT_TRAP_GATE_FLAGS, ISR(general_prot), 0);
    idt_set_desc(0xE, IDT_TRAP_GATE_FLAGS, ISR(page_fault), 0);
    idt_load();
}

void
processor_halt(void)
{
    __ASMV("cli; hlt");
}

__weak void
processor_init(struct processor *processor)
{
    gdt_load(processor->machdep.gdtr);
    interrupts_init(processor);
}

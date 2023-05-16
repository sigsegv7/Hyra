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

#include <arch/x86/exceptions.h>
#include <arch/x86/intr.h>
#include <arch/x86/idt.h>
#include <sys/panic.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#if defined(__x86_64__)

#define register_exception(vec, isr) \
    idt_set_desc(vec, IDT_TRAP_GATE_FLAGS, (uintptr_t)isr, 0)

static const char *exception_names[] = {
    [VECTOR_DIVIDE_ERROR]       = "Divide error",
    [VECTOR_DEBUG_EXCEPTION]    = "Debug exception",
    [VECTOR_BREAKPOINT]         = "Breakpoint (unimplemented)",
    [VECTOR_OVERFLOW]           = "Overflow",
    [VECTOR_BR]                 = "BOUND Range Exceeded",
    [VECTOR_INVALID_OPCODE]     = "Invalid opcode",
    [VECTOR_NM]                 = "#NM",
    [VECTOR_DOUBLE_FAULT]       = "Double fault",
    [VECTOR_INVALID_TSS]        = "Invalid TSS",
    [VECTOR_SS]                 = "Stack segment fault",
    [VECTOR_GENERAL_PROTECTION] = "General protection fault",
    [VECTOR_PAGE_FAULT]         = "Page fault" 
};

static inline void
handle_fatal(uint8_t vector, uintptr_t rip)
{
    panic("Caught %s (rip=0x%x)\n",
          exception_names[vector], rip);

    __builtin_unreachable();
}

__isr static void
divide_err(uintptr_t *sf)
{
    handle_fatal(VECTOR_DIVIDE_ERROR, sf[0]);
    __builtin_unreachable();
}

__isr static void
debug_exception(uintptr_t *sf)
{
    handle_fatal(VECTOR_DEBUG_EXCEPTION, sf[0]);
    __builtin_unreachable();
}

__isr static void
breakpoint(uintptr_t *sf)
{
    handle_fatal(VECTOR_BREAKPOINT, sf[0]);
    __builtin_unreachable();
}

__isr static void
overflow(uintptr_t *sf)
{
    handle_fatal(VECTOR_OVERFLOW, sf[0]);
    __builtin_unreachable();
}

__isr static void
handle_br(uintptr_t *sf)
{
    handle_fatal(VECTOR_OVERFLOW, sf[0]);
    __builtin_unreachable();
}

__isr static void
invalid_opcode(uintptr_t *sf)
{
    handle_fatal(VECTOR_INVALID_OPCODE, sf[0]);
    __builtin_unreachable();
}

__isr static void
handle_nm(uintptr_t *sf)
{
    handle_fatal(VECTOR_NM, sf[0]);
    __builtin_unreachable();
}

__isr static void
double_fault(uintptr_t *sf)
{
    handle_fatal(VECTOR_DOUBLE_FAULT, sf[1]);
    __builtin_unreachable();
}

__isr static void
invalid_tss(uintptr_t *sf)
{
    handle_fatal(VECTOR_INVALID_TSS, sf[1]);
    __builtin_unreachable();
}

__isr static void
stack_segment_fault(uintptr_t *sf)
{
    handle_fatal(VECTOR_SS, sf[1]);
    __builtin_unreachable();
}

__isr static void
general_protection(uintptr_t *sf)
{
    handle_fatal(VECTOR_GENERAL_PROTECTION, sf[1]);
    __builtin_unreachable();
}

__isr static void
page_fault(uintptr_t *sf)
{
    handle_fatal(VECTOR_PAGE_FAULT, sf[1]);
    __builtin_unreachable();
}

void
exceptions_init(void)
{
    register_exception(VECTOR_DIVIDE_ERROR, divide_err);
    register_exception(VECTOR_DEBUG_EXCEPTION, debug_exception);
    register_exception(VECTOR_BREAKPOINT, breakpoint);
    register_exception(VECTOR_OVERFLOW, overflow);
    register_exception(VECTOR_BR, handle_br);
    register_exception(VECTOR_INVALID_OPCODE,invalid_opcode);
    register_exception(VECTOR_NM, handle_nm);
    register_exception(VECTOR_DOUBLE_FAULT, double_fault);
    register_exception(VECTOR_INVALID_TSS, invalid_tss);
    register_exception(VECTOR_DIVIDE_ERROR, divide_err);
    register_exception(VECTOR_SS, stack_segment_fault);
    register_exception(VECTOR_GENERAL_PROTECTION, general_protection);
    register_exception(VECTOR_PAGE_FAULT, page_fault);
}

#endif      /* defined(__x86_64__) */

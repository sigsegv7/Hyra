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

#include <machine/frameasm.h>
#include <machine/trap.h>
#include <sys/cdefs.h>

__KERNEL_META "$Hyra$: trap.S, Ian Marco Moffett, \
               Trap handling routines"

.text
.globl breakpoint_handler
breakpoint_handler:
    push_trapframe_ec $TRAP_BREAKPOINT

    handle_trap

    /* TODO */
    cli
    hlt

.globl arith_err
arith_err:
    push_trapframe_ec $TRAP_ARITH_ERR

    handle_trap

    /* TODO */
    cli
    hlt

.globl overflow
overflow:
    push_trapframe_ec $TRAP_OVERFLOW

    handle_trap

    /* TODO */
    cli
    hlt

.globl bound_range
bound_range:
    push_trapframe_ec $TRAP_BOUND_RANGE

    handle_trap

    /* TODO */
    cli
    hlt

.globl invl_op
invl_op:
    push_trapframe_ec $TRAP_INVLOP

    handle_trap

    /* TODO */
    cli
    hlt

.globl double_fault
double_fault:
    push_trapframe_ec $TRAP_DOUBLE_FAULT

    handle_trap

    /* TODO */
    cli
    hlt

.globl invl_tss
invl_tss:
    push_trapframe_ec $TRAP_INVLTSS

    handle_trap

    /* TODO */
    cli
    hlt

.globl segnp
segnp:
    push_trapframe_ec $TRAP_SEGNP

    handle_trap

    /* TODO */
    cli
    hlt

.globl general_prot
general_prot:
    push_trapframe_ec $TRAP_PROTFLT

    handle_trap

    /* TODO */
    cli
    hlt

.globl page_fault
page_fault:
    push_trapframe_ec $TRAP_PAGEFLT

    handle_trap

    /* TODO */
    cli
    hlt

.globl nmi
nmi:
    push_trapframe_ec $TRAP_NMI

    handle_trap

    /* TODO */
    cli
    hlt

.globl ss_fault
ss_fault:
    push_trapframe_ec $TRAP_SS

    handle_trap

    /* TODO */
    cli
    hlt

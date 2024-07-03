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

    .text
    .globl breakpoint_handler
TRAPENTRY(breakpoint_handler, $TRAP_BREAKPOINT)

    .globl arith_err
TRAPENTRY(arith_err, $TRAP_ARITH_ERR)

    .globl overflow
TRAPENTRY(overflow, $TRAP_OVERFLOW)

    .globl bound_range
TRAPENTRY(bound_range, $TRAP_BOUND_RANGE)

    .globl invl_op
TRAPENTRY(invl_op, $TRAP_INVLOP)

    .globl double_fault
TRAPENTRY(double_fault, $TRAP_DOUBLE_FAULT)

    .globl invl_tss
TRAPENTRY(invl_tss, $TRAP_INVLTSS)

    .globl segnp
TRAPENTRY(segnp, $TRAP_SEGNP)

    .globl general_prot
TRAPENTRY(general_prot, $TRAP_PROTFLT)

    .globl page_fault
TRAPENTRY(page_fault, $TRAP_PAGEFLT)

    .globl nmi
TRAPENTRY(nmi, $TRAP_NMI)

    .globl ss_fault
TRAPENTRY(ss_fault, $TRAP_SS)

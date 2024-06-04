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

#ifndef _MACHINE_TRAP_H_
#define _MACHINE_TRAP_H_

#if !defined(__ASSEMBLER__)
#include <machine/frame.h>
#endif

#define TRAP_NONE           0       /* Used for general interrupts */
#define TRAP_BREAKPOINT     1       /* Breakpoint */
#define TRAP_ARITH_ERR      2       /* Arithmetic error (e.g division by 0) */
#define TRAP_OVERFLOW       3       /* Overflow */
#define TRAP_BOUND_RANGE    4       /* BOUND range exceeded */
#define TRAP_INVLOP         5       /* Invalid opcode */
#define TRAP_DOUBLE_FAULT   6       /* Double fault */
#define TRAP_INVLTSS        7       /* Invalid TSS */
#define TRAP_SEGNP          8       /* Segment not present */
#define TRAP_PROTFLT        9       /* General protection */
#define TRAP_PAGEFLT        10      /* Page fault */
#define TRAP_NMI            11      /* Non-maskable interrupt */
#define TRAP_SS             12      /* Stack-segment fault */

#if !defined(__ASSEMBLER__)

void breakpoint_handler(void *sf);
void arith_err(void *sf);
void overflow(void *sf);
void bound_range(void *sf);
void invl_op(void *sf);
void double_fault(void *sf);
void invl_tss(void *sf);
void segnp(void *sf);
void general_prot(void *sf);
void page_fault(void *sf);
void nmi(void *sf);
void ss_fault(void *sf);
void trap_handler(struct trapframe *tf);

#endif  /* !__ASSEMBLER__ */
#endif  /* !_MACHINE_TRAP_H_ */

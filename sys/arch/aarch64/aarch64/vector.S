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

#include <machine/frameasm.h>

// Vector table entries are aligned at 128 bytes
// giving us 32 exception entries
.macro ventry label
    .align 7
    b \label
.endm

    .text
x_sync_elx:
    PUSH_XFRAME(TRAPNO_XSYNC)   // Synchronous: sp+top @ X0
    bl handle_exception         // Handle the exception
    POP_XFRAME()                // Pop the trapframe
1:  hlt #0                      // TODO
    b 1b

x_irq_elx:
    PUSH_XFRAME(TRAPNO_XIRQ)    // IRQ: sp+top @ X0
    bl handle_exception         // Handle the exception
    POP_XFRAME()                // Pop the trapframe
1:  hlt #0                      // TODO
    b 1b

x_fiq_elx:
    PUSH_XFRAME(TRAPNO_XFIQ)    // FIQ: sp+top @ X0
    bl handle_exception         // Handle the exception
    POP_XFRAME()                // Pop the trapframe
1:  hlt #0
    b 1b

x_serr_elx:
    PUSH_XFRAME(TRAPNO_XSERR)   // SERR: sp+top @ X0
    bl handle_exception         // Handle the exception
    POP_XFRAME()                // Pop the trapframe
1:  hlt #0                      // TODO
    b 1b

x_unimpl:
1:  hlt #0
    b 1b

    .align 11         // Table aligned @ 2 KiB
    .globl __vectab
__vectab:
    // From current EL (w/ SP_EL0)
    ventry x_sync_elx
    ventry x_irq_elx
    ventry x_fiq_elx
    ventry x_serr_elx

    // From current EL (w/ SP_ELx > 0)
    ventry x_sync_elx
    ventry x_irq_elx
    ventry x_fiq_elx
    ventry x_serr_elx

    // Lower EL with faulting code in AARCH64
    ventry x_sync_elx
    ventry x_irq_elx
    ventry x_fiq_elx
    ventry x_serr_elx

    ventry x_unimpl
    ventry x_unimpl
    ventry x_unimpl
    ventry x_unimpl

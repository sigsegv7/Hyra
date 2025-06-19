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

#ifndef _MACHINE_FRAMEASM_H_
#define _MACHINE_FRAMEASM_H_

/* XXX: Must be 16-byte aligned!!! */
#define XFRAME_STACK_SIZE (38 * 8)

/* Trap numbers */
#define TRAPNO_UNKNOWN #0
#define TRAPNO_XSYNC   #1     /* Synchronous */
#define TRAPNO_XIRQ    #2     /* IRQ */
#define TRAPNO_XFIQ    #3     /* FIQ */
#define TRAPNO_XSERR   #4     /* System error */

#define PUSH_XFRAME(TRAPNO)          \
    sub sp, sp, #XFRAME_STACK_SIZE ; \
    stp x30, x29, [sp, #(0 * 8)]   ; \
    stp x28, x27, [sp, #(2 * 8)]   ; \
    stp x26, x25, [sp, #(4 * 8)]   ; \
    stp x24, x23, [sp, #(6 * 8)]   ; \
    stp x22, x21, [sp, #(8 * 8)]   ; \
    stp x20, x19, [sp, #(10 * 8)]  ; \
    stp x18, x17, [sp, #(12 * 8)]  ; \
    stp x16, x15, [sp, #(14 * 8)]  ; \
    stp x14, x13, [sp, #(16 * 8)]  ; \
    stp x12, x11, [sp, #(18 * 8)]  ; \
    stp x10, x9, [sp, #(20 * 8)]   ; \
    stp x8, x7, [sp, #(22 * 8)]    ; \
    stp x6, x5, [sp, #(24 * 8)]    ; \
    stp x4, x3, [sp, #(26 * 8)]    ; \
    stp x2, x1, [sp, #(28 * 8)]    ; \
    str x0, [sp, #(30 * 8)]        ; \
                                   ; \
    mrs x0, elr_el1                ; \
    str x0, [sp, #(31 * 8)]        ; \
    mrs x0, esr_el1                ; \
    str x0, [sp, #(32 * 8)]        ; \
    mov x0, TRAPNO                 ; \
    str x0, [sp, #(33 * 8)]        ; \
    mov x0, sp

#define POP_XFRAME()                  \
    ldr x0, [sp, #(30 * 8)]         ; \
    ldp x2, x1, [sp, #(28 * 8)]     ; \
    ldp x4, x3, [sp, #(26 * 8)]     ; \
    ldp x6, x5, [sp, #(24 * 8)]     ; \
    ldp x8, x7, [sp, #(22 * 8)]     ; \
    ldp x10, x9, [sp, #(20 * 8)]    ; \
    ldp x12, x11, [sp, #(18 * 8)]   ; \
    ldp x14, x13, [sp, #(16 * 8)]   ; \
    ldp x16, x15, [sp, #(14 * 8)]   ; \
    ldp x18, x17, [sp, #(12 * 8)]   ; \
    ldp x20, x19, [sp, #(10 * 8)]   ; \
    ldp x22, x21, [sp, #(8 * 8)]    ; \
    ldp x24, x23, [sp, #(6 * 8)]    ; \
    ldp x26, x25, [sp, #(4 * 8)]    ; \
    ldp x28, x27, [sp, #(2 * 8)]    ; \
    ldp x30, x29, [sp, #(0 * 8)]    ; \
    add sp, sp, #XFRAME_STACK_SIZE

#endif  /* !_MACHINE_FRAMEASM_H_ */

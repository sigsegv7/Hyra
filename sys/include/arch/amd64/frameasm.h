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

#define ALIGN_TEXT .align 8, 0x90

/*
 * If the interrupt has an error code, this macro shall
 * be used to create the trapframe.
 *
 * XXX: A trapframe created with this must be popped with
 *      pop_trapframe_ec
 */
#define PUSH_TRAPFRAME_EC(TRAPNO) \
    push %r15                   ; \
    push %r14                   ; \
    push %r13                   ; \
    push %r12                   ; \
    push %r11                   ; \
    push %r10                   ; \
    push %r9                    ; \
    push %r8                    ; \
    push %rbp                   ; \
    push %rdi                   ; \
    push %rsi                   ; \
    push %rbx                   ; \
    push %rdx                   ; \
    push %rcx                   ; \
    push %rax                   ; \
    push TRAPNO

/*
 * If the interrupt has an error code, this macro shall
 * be used to cleanup the trapframe.
 */
#define POP_TRAPFRAME_EC                  \
    add $8, %rsp        /* Trapno */    ; \
    pop %rax                            ; \
    pop %rcx                            ; \
    pop %rdx                            ; \
    pop %rbx                            ; \
    pop %rsi                            ; \
    pop %rdi                            ; \
    pop %rbp                            ; \
    pop %r8                             ; \
    pop %r9                             ; \
    pop %r10                            ; \
    pop %r11                            ; \
    pop %r12                            ; \
    pop %r13                            ; \
    pop %r14                            ; \
    pop %r15

/*
 * If the interrupt has no error code, this macro
 * shall be used to create the trapframe.
 *
 * XXX: A trapframe created with this must be popped
 *      with pop_trapframe
 */
#define PUSH_TRAPFRAME(TRAPNO)   \
    push $0                    ; \
    PUSH_TRAPFRAME_EC(TRAPNO)

/*
 * If the interrupt has no error code, this macro shall
 * be used to cleanup the trapframe.
 */
#define POP_TRAPFRAME    \
    POP_TRAPFRAME_EC   ; \
    add $8, %rsp

/*
 * Generic interrupt entry.
 */
#define INTRENTRY(ENTLABEL, HANDLER)    \
    ENTLABEL:                           \
        testq $0x3, 8(%rsp)           ; \
        jz 1f                         ; \
        lfence                        ; \
        swapgs                        ; \
    1:  PUSH_TRAPFRAME($0)            ; \
        mov %rsp, %rdi                ; \
        call HANDLER                  ; \
        POP_TRAPFRAME                 ; \
        testq $0x3, 8(%rsp)           ; \
        jz 2f                         ; \
        lfence                        ; \
        swapgs                        ; \
    2:  iretq

/*
 * Trap entry where an error code is on
 * the stack.
 */
#define TRAPENTRY_EC(ENTLABEL, TRAPNO)  \
    ENTLABEL:                         ; \
        cli                           ; \
        testq $0x3, 16(%rsp)          ; \
        jz 1f                         ; \
        lfence                        ; \
        swapgs                        ; \
    1:  PUSH_TRAPFRAME_EC(TRAPNO)     ; \
        mov %rsp, %rdi                ; \
        call trap_handler             ; \
        POP_TRAPFRAME_EC              ; \
        testq $0x3, 16(%rsp)          ; \
        jz 2f                         ; \
        lfence                        ; \
        swapgs                        ; \
    2:  sti                           ; \
        iretq

/*
 * Trap entry where no error code is on
 * the stack.
 */
#define TRAPENTRY(ENTLABEL, TRAPNO)     \
    ENTLABEL:                         ; \
        cli                           ; \
        testq $0x3, 8(%rsp)           ; \
        jz 1f                         ; \
        lfence                        ; \
        swapgs                        ; \
    1:  PUSH_TRAPFRAME(TRAPNO)        ; \
        mov %rsp, %rdi                ; \
        call trap_handler             ; \
        POP_TRAPFRAME                 ; \
        testq $0x3, 8(%rsp)           ; \
        jz 2f                         ; \
        lfence                        ; \
        swapgs                        ; \
    2:  sti                           ; \
        iretq

#endif  /* !_MACHINE_FRAMEASM_H_ */

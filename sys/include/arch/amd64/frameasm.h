/*
 * Copyright (c) 2023 Ian Marco Moffett and the Osmora Team.
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

#ifndef _AMD64_FRAMEASM_H_
#define _AMD64_FRAMEASM_H_

/*
 * XXX: Before this macro is invoked,
 *      you should determine if an error
 *      code will be present already on the
 *      stack. If not, push a null qword as
 *      padding (e.g push $0).
 *
 *      There *must* be a value used
 *      as an error code whether that be
 *      a real error code or just padding.
 *
 *      Failing to do so will result in
 *      undefined behaviour.
 *
 */
.macro push_trapframe trapno
    push %r15
    push %r14
    push %r13
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    push %rbp
    push %rdi
    push %rsi
    push %rbx
    push %rdx
    push %rcx
    push %rax
    push \trapno
.endm

.macro pop_trapframe
    add $8, %rsp        /* Trapno */
    pop %rax
    pop %rcx
    pop %rdx
    pop %rbx
    pop %rsi
    pop %rdi
    pop %rbp
    pop %r8
    pop %r9
    pop %r10
    pop %r12
    pop %r13
    pop %r14
    pop %r15
    add $8, %rsp        /* Pop error code */
    iretq
.endm
#endif  /* !_AMD64_FRAMEASM_H_ */

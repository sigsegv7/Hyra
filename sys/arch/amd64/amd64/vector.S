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

#define IDT_INT_GATE 0x8E

.macro IDT_SET_VEC vec, sym
    mov $\vec, %rdi
    mov $IDT_INT_GATE, %rsi
    lea \sym(%rip), %rdx
    xor %rcx, %rcx
    call idt_set_desc
.endm

    .text
    ALIGN_TEXT
ioapic_common_func:
    xor %rcx, %rcx              // Clear counter
.walk:                          // Walk the handlers
    lea g_intrs(%rip), %rbx     // Grab table to RBX
    lea (%rbx, %rcx, 8), %rbx   // g_intrs + (8 * rcx)
    mov (%rbx), %rdx            // Grab the intr_hand
    or %rdx, %rdx               // No more?
    jz 1f                       // Nope, return

    mov (%rdx), %rbx            // intr_hand.func
    add $16, %rdx               // Get interrupt data
    mov %rdx, %rdi              // Pass the interrupt data
    push %rcx                   // Save our counter
    push %rdx
    call *%rbx                  // Call the handler
    pop %rdx
    pop %rcx                    // Restore our counter
    or %rax, %rax               // Was it theirs? (RET >= 1)
    jnz handled                 // Yes, we are done.
1:  inc %rcx                    // Next
    cmp $256, %rcx              // Did we reach the end?
    jl .walk                    // Nope, keep going
    jmp done                    // Out of entries
handled:
    sub $8, %rdx
    addq $1, (%rdx)
done:
    call lapic_eoi
    retq

    .globl pin_isr_load
pin_isr_load:
    IDT_SET_VEC 35, ioapic_edge_0
    IDT_SET_VEC 36, ioapic_edge_1
    IDT_SET_VEC 37, ioapic_edge_2
    IDT_SET_VEC 38, ioapic_edge_3
    IDT_SET_VEC 39, ioapic_edge_4
    IDT_SET_VEC 40, ioapic_edge_5
    IDT_SET_VEC 41, ioapic_edge_6
    IDT_SET_VEC 42, ioapic_edge_7
    IDT_SET_VEC 43, ioapic_edge_8
    IDT_SET_VEC 44, ioapic_edge_9
    IDT_SET_VEC 45, ioapic_edge_10
    IDT_SET_VEC 46, ioapic_edge_11
    IDT_SET_VEC 47, ioapic_edge_12
    IDT_SET_VEC 48, ioapic_edge_13
    IDT_SET_VEC 49, ioapic_edge_14
    IDT_SET_VEC 50, ioapic_edge_15
    IDT_SET_VEC 51, ioapic_edge_16
    IDT_SET_VEC 52, ioapic_edge_17
    IDT_SET_VEC 53, ioapic_edge_18
    IDT_SET_VEC 54, ioapic_edge_19
    IDT_SET_VEC 55, ioapic_edge_20
    IDT_SET_VEC 56, ioapic_edge_21
    IDT_SET_VEC 57, ioapic_edge_22
    IDT_SET_VEC 58, ioapic_edge_23
    IDT_SET_VEC 59, ioapic_edge_24
    IDT_SET_VEC 60, ioapic_edge_25
    IDT_SET_VEC 61, ioapic_edge_26
    IDT_SET_VEC 62, ioapic_edge_27
    IDT_SET_VEC 63, ioapic_edge_28
    IDT_SET_VEC 64, ioapic_edge_29
    IDT_SET_VEC 65, ioapic_edge_30
    IDT_SET_VEC 66, ioapic_edge_31
    IDT_SET_VEC 67, ioapic_edge_32
    IDT_SET_VEC 68, ioapic_edge_33
    IDT_SET_VEC 69, ioapic_edge_34
    IDT_SET_VEC 70, ioapic_edge_35
    IDT_SET_VEC 71, ioapic_edge_36
    IDT_SET_VEC 72, ioapic_edge_37
    IDT_SET_VEC 73, ioapic_edge_38
    IDT_SET_VEC 74, ioapic_edge_39
    IDT_SET_VEC 75, ioapic_edge_40
    IDT_SET_VEC 76, ioapic_edge_41
    IDT_SET_VEC 77, ioapic_edge_42
    IDT_SET_VEC 78, ioapic_edge_43
    IDT_SET_VEC 79, ioapic_edge_44
    IDT_SET_VEC 80, ioapic_edge_45
    IDT_SET_VEC 81, ioapic_edge_46
    IDT_SET_VEC 82, ioapic_edge_47
    IDT_SET_VEC 83, ioapic_edge_48
    IDT_SET_VEC 84, ioapic_edge_49
    IDT_SET_VEC 85, ioapic_edge_50
    IDT_SET_VEC 86, ioapic_edge_51
    IDT_SET_VEC 87, ioapic_edge_52
    IDT_SET_VEC 88, ioapic_edge_53
    IDT_SET_VEC 89, ioapic_edge_54
    IDT_SET_VEC 90, ioapic_edge_55
    IDT_SET_VEC 91, ioapic_edge_56
    IDT_SET_VEC 92, ioapic_edge_57
    IDT_SET_VEC 93, ioapic_edge_58
    IDT_SET_VEC 94, ioapic_edge_59
    IDT_SET_VEC 95, ioapic_edge_60
    IDT_SET_VEC 96, ioapic_edge_61
    IDT_SET_VEC 97, ioapic_edge_62
    IDT_SET_VEC 97, ioapic_edge_63
    ret

/* I/O APIC edge ISRs */
INTRENTRY(ioapic_edge_0, ioapic_common_func)
INTRENTRY(ioapic_edge_1, ioapic_common_func)
INTRENTRY(ioapic_edge_2, ioapic_common_func)
INTRENTRY(ioapic_edge_3, ioapic_common_func)
INTRENTRY(ioapic_edge_4, ioapic_common_func)
INTRENTRY(ioapic_edge_5, ioapic_common_func)
INTRENTRY(ioapic_edge_6, ioapic_common_func)
INTRENTRY(ioapic_edge_7, ioapic_common_func)
INTRENTRY(ioapic_edge_8, ioapic_common_func)
INTRENTRY(ioapic_edge_9, ioapic_common_func)
INTRENTRY(ioapic_edge_10, ioapic_common_func)
INTRENTRY(ioapic_edge_11, ioapic_common_func)
INTRENTRY(ioapic_edge_12, ioapic_common_func)
INTRENTRY(ioapic_edge_13, ioapic_common_func)
INTRENTRY(ioapic_edge_14, ioapic_common_func)
INTRENTRY(ioapic_edge_15, ioapic_common_func)
INTRENTRY(ioapic_edge_16, ioapic_common_func)
INTRENTRY(ioapic_edge_17, ioapic_common_func)
INTRENTRY(ioapic_edge_18, ioapic_common_func)
INTRENTRY(ioapic_edge_19, ioapic_common_func)
INTRENTRY(ioapic_edge_20, ioapic_common_func)
INTRENTRY(ioapic_edge_21, ioapic_common_func)
INTRENTRY(ioapic_edge_22, ioapic_common_func)
INTRENTRY(ioapic_edge_23, ioapic_common_func)
INTRENTRY(ioapic_edge_24, ioapic_common_func)
INTRENTRY(ioapic_edge_25, ioapic_common_func)
INTRENTRY(ioapic_edge_26, ioapic_common_func)
INTRENTRY(ioapic_edge_27, ioapic_common_func)
INTRENTRY(ioapic_edge_28, ioapic_common_func)
INTRENTRY(ioapic_edge_29, ioapic_common_func)
INTRENTRY(ioapic_edge_30, ioapic_common_func)
INTRENTRY(ioapic_edge_31, ioapic_common_func)
INTRENTRY(ioapic_edge_32, ioapic_common_func)
INTRENTRY(ioapic_edge_33, ioapic_common_func)
INTRENTRY(ioapic_edge_34, ioapic_common_func)
INTRENTRY(ioapic_edge_35, ioapic_common_func)
INTRENTRY(ioapic_edge_36, ioapic_common_func)
INTRENTRY(ioapic_edge_37, ioapic_common_func)
INTRENTRY(ioapic_edge_38, ioapic_common_func)
INTRENTRY(ioapic_edge_39, ioapic_common_func)
INTRENTRY(ioapic_edge_40, ioapic_common_func)
INTRENTRY(ioapic_edge_41, ioapic_common_func)
INTRENTRY(ioapic_edge_42, ioapic_common_func)
INTRENTRY(ioapic_edge_43, ioapic_common_func)
INTRENTRY(ioapic_edge_44, ioapic_common_func)
INTRENTRY(ioapic_edge_45, ioapic_common_func)
INTRENTRY(ioapic_edge_46, ioapic_common_func)
INTRENTRY(ioapic_edge_47, ioapic_common_func)
INTRENTRY(ioapic_edge_48, ioapic_common_func)
INTRENTRY(ioapic_edge_49, ioapic_common_func)
INTRENTRY(ioapic_edge_50, ioapic_common_func)
INTRENTRY(ioapic_edge_51, ioapic_common_func)
INTRENTRY(ioapic_edge_52, ioapic_common_func)
INTRENTRY(ioapic_edge_53, ioapic_common_func)
INTRENTRY(ioapic_edge_54, ioapic_common_func)
INTRENTRY(ioapic_edge_55, ioapic_common_func)
INTRENTRY(ioapic_edge_56, ioapic_common_func)
INTRENTRY(ioapic_edge_57, ioapic_common_func)
INTRENTRY(ioapic_edge_58, ioapic_common_func)
INTRENTRY(ioapic_edge_59, ioapic_common_func)
INTRENTRY(ioapic_edge_60, ioapic_common_func)
INTRENTRY(ioapic_edge_61, ioapic_common_func)
INTRENTRY(ioapic_edge_62, ioapic_common_func)
INTRENTRY(ioapic_edge_63, ioapic_common_func)

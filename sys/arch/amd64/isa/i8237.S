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

#include <sys/errno.h>
#include <machine/isa/i8237.h>

    .section .rodata
t_set_addr:
    .quad set_addr_0
    .quad set_addr_1
    .quad set_addr_2
    .quad set_addr_3
t_set_count:
    .quad set_count_0
    .quad set_count_1
    .quad set_count_2
    .quad set_count_3

    .text
    .globl i8237_set_addr
    .type i8237_set_addr, @function
i8237_set_addr:
    /*
     * i8237_set_addr(u8 channel, u16 addr) -> u8
     */

    /* If (RDI > 3) then return -EINVAL */
    cmp $3, %rdi
    jg 1f

    /* Set the address at the target channel */
    lea t_set_addr(%rip), %rbx
    mov (%rbx, %rdi, 8), %rbx
    jmp *%rbx
1:
    mov $-EINVAL, %rax
    retq

    .text
    .globl i8237_set_count
    .type i8237_set_count, @function
i8237_set_count:
    /*
     * i8237_set_count(u8 channel, u16 count) -> u8
     */

    /* If (RDI > 3) then return -EINVAL */
    cmp $3, %rdi
    jg 1f

    /* Set the count at the target channel */
    lea t_set_count(%rip), %rbx
    mov (%rbx, %rdi, 8), %rbx
    jmp *%rbx
1:
    mov $-EINVAL, %rax
    retq

    .text
    .globl i8237_set_mode
    .type i8237_set_mode, @function
i8237_set_mode:
    /*
     * i8237_set_mode(u8 channel, u8 mode) -> u8
     */

    /* If (RDI > 3) then return -EINVAL */
    cmp $3, %rdi
    jg 1f

    /* Set channel select bits */
    mov %dil, %al
    andb $3, %al

    /* Set channel mode bits */
    mov %si, %bx
    shlw $2, %bx
    or %bl, %al

    /* Write the mode then we are done */
    outb %al, $0x0B
    xor %rax, %rax
    retq
1:
    mov $-EINVAL, %rax
    retq

/* -- Acquire the global lock -- */
lock_acquire:
    lock btsl $0, lock(%rip)
    jc .lock_spin
    retq
.lock_spin:
    testb $1, lock
    jc .lock_spin
    jmp lock_acquire

/* -- Release the global lock -- */
lock_release:
    movb $0, lock(%rip)
    retq

/* -- Main port setting logic (value needed in %RDI) -- */
.macro set_port port
    call lock_acquire
    push %rdi

    /* Ensure the flip-flop is in a known state */
    mov $0xFF, %al
    out %al, $0x0C

    /* Write the first 8 bits and shift */
    mov %dil, %al
    shl $8, %di
    out %al, \port

    /* Write the last 8 bits */
    mov %dil, %al
    out %al, \port
    pop %rdi
    call lock_release
    retq
.endm

/* set_addr_0(u16 addr) */
set_addr_0:
    set_port $DMA_ADDR_0

    /* Success */
    xor %rax, %rax
    retq

/* set_addr_1(u16 addr) */
set_addr_1:
    set_port $DMA_ADDR_1

    /* Success */
    xor %rax, %rax
    retq

/* set_addr_2(u16 addr) */
set_addr_2:
    set_port $DMA_ADDR_2

    /* Success */
    xor %rax, %rax
    retq

/* set_addr_3(u16 addr) */
set_addr_3:
    set_port $DMA_ADDR_3

    /* Success */
    xor %rax, %rax
    retq

/* set_count_0(u16 count) */
set_count_0:
    set_port $DMA_COUNT_0

    /* Success */
    xor %rax, %rax
    retq

/* set_count_1(u16 count) */
set_count_1:
    call lock_acquire
    set_port $DMA_COUNT_1
    call lock_release

    /* Success */
    xor %rax, %rax
    retq

/* set_count_2(u16 count) */
set_count_2:
    set_port $DMA_COUNT_2

    /* Success */
    xor %rax, %rax
    retq

/* set_count_3(u16 count) */
set_count_3:
    call lock_acquire
    set_port $DMA_COUNT_3

    /* Success */
    xor %rax, %rax
    retq

/* -- Global spinlock -- */

    .section .bss
    .align 64
lock:
    .long 0

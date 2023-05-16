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

#if defined(__x86_64__)

.globl amd64_enable_sse
.globl amd64_enable_avx
.globl amd64_fast_memcpy

.section .text
/* Same thing as amd64_enable_avx but for SSE */
amd64_enable_sse:
    mov $0x1, %eax
    cpuid

    mov $0x1, %ebx
    shl $25, %ebx

    test %ebx, %edx
    jz 1f

    /* Check if SSE is supported */

    /* Prepare ~(1 << 2) in RAX */
    mov $1, %rax
    shl $2, %rax
    not %rax
    
    /* Clear CR0.EM */
    mov %cr0, %rdx
    and %rax, %rdx
    mov %rdx, %cr0

    /* Prepare (1 << 2) in RAX */
    mov $1, %rax
    shl $1, %rax

    /* Set CR0.MP */
    mov %cr0, %rdx
    or %rax, %rdx
    mov %rdx, %cr0

    /* Prepare value with bit 9 and 10 set in RAX */
    mov $3, %rax
    shl $9, %rax

    /* Set CR4.OSFXSR and CR4 OSXMMEXCPT (bit 9, 10) */
    mov %cr4, %rdx
    or %rax, %rax
    mov %rdx, %cr4

    /* SSE should be enabled */
    xor %rax, %rax
    retq
1:
    mov $1, %rax
    retq

/*
 * Attempts to enable AVX
 * if supported.
 * 
 * Returns nonzero value
 * on failure.
 */

amd64_enable_avx:
    mov $0x1, %eax
    cpuid
    
    /* Check if the CPUID result says AVX is supported */
    mov $1, %ebx
    shl $28, %ebx
    test %ebx, %ecx
    jz 1f

    /* Enable OSXSAVE */
    mov $1, %rbx
    shl $18, %rbx

    /* Bit 18 of CR4 enables OSXSAVE */
    mov %cr4, %rdx
    or %rbx, %rdx
    mov %rdx, %cr4
    
    /* Enable AVX */
    xor %rcx, %rcx
    xgetbv
    or $7, %eax
    xsetbv

    xor %rax, %rax
    retq
1:
    /* AVX is not supported */
    mov $1, %rax
    retq

#endif

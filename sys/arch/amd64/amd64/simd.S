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

    .text
    .globl simd_init
simd_init:
    /*
     * Enable SIMD, if SSE and AVX is supported,
     * a value of zero is returned. If SSE is
     * supported yet AVX is not, a value of one
     * is returned. However, if none are supported,
     * this routine returns -1.
     */

    // Do we support SSE?
    mov $1, %eax
    cpuid
    bt $25, %edx
    jnc .sse_not_sup

    mov %cr0, %rax    // Old CR0 -> EAX
    and $0xFFFB, %ax  // Disable co-processor emulation
    or $0x02, %ax     // Enable co-processor monitoring
    mov %rax, %cr0    // Update CR0 with new flags

    mov %cr4, %rax    // Old CR4 -> EAX
    or $0x200, %ax    // Enable FXSAVE/FXRSTOR
    or $0x400, %ax    // Enable SIMD FP exceptions
    mov %rax, %cr4    // Update CR4 with new flags

    mov $1, %eax      // LEAF 1
    cpuid             // Bit 28 of ECX indicates AVX support
    mov $3, %eax      // We need to check two bits
    shl $27, %eax     // Which are ECX.OSXSAVE and ECX.AVX
    test %eax, %ecx   // Are XSAVE and AVX supported?
    jnc .avx_not_sup  // Nope, just continue

    // Enable AVX
    xor %rcx, %rcx    // Select XCR0
    xgetbv            // Load extended control register
    or $0x07, %eax    // Set AVX + SSE bits
    xsetbv            // Store new flags
    xor %rax, %rax    // Everything is good
    retq              // Return back to caller (RETURN)
.sse_not_sup:
    mov $-1, %rax
    retq
.avx_not_sup:
    mov $1, %rax
    retq

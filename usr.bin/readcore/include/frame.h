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

#ifndef FRAME_H_
#define FRAME_H_

#include <sys/types.h>

struct core;

#if defined(__x86_64__)
struct core_frame {
    uint64_t trapno;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};
#elif defined(__aarch64__)
struct core_frame {
    lreg_t x30;
    lreg_t x29;
    lreg_t x28;
    lreg_t x27;
    lreg_t x26;
    lreg_t x25;
    lreg_t x24;
    lreg_t x23;
    lreg_t x22;
    lreg_t x21;
    lreg_t x20;
    lreg_t x19;
    lreg_t x18;
    lreg_t x17;
    lreg_t x16;
    lreg_t x15;
    lreg_t x14;
    lreg_t x13;
    lreg_t x12;
    lreg_t x11;
    lreg_t x10;
    lreg_t x9;
    lreg_t x8;
    lreg_t x7;
    lreg_t x6;
    lreg_t x5;
    lreg_t x4;
    lreg_t x3;
    lreg_t x2;
    lreg_t x1;
    lreg_t x0;
    lreg_t elr;
    lreg_t esr;
    frament_t trapno;
};
#else
struct core_frame {
    uint64_t data[30];
};
#endif

void core_dumpframe(const struct core *core);

#endif  /* !FRAME_H_ */

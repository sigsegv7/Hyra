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

#ifndef _AMD64_FRAME_H_
#define _AMD64_FRAME_H_

#if !defined(__ASSEMBLER__)

#include <sys/types.h>

struct trapframe {
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
    /* Pushed by hardware */
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

#define init_frame(FRAME, IP, SP)   \
    (FRAME)->rip        = IP;       \
    (FRAME)->cs         = 0x08;     \
    (FRAME)->rflags     = 0x202;    \
    (FRAME)->rsp        = SP;       \
    (FRAME)->ss         = 0x10;     \

#define init_frame_user(FRAME, IP, SP)      \
    (FRAME)->rip = IP;                      \
    (FRAME)->cs = 0x18 | 3;                 \
    (FRAME)->rflags = 0x202;                \
    (FRAME)->rsp = SP;                      \
    (FRAME)->ss = 0x20 | 3;                 \

#define set_frame_sp(FRAME, SP) (FRAME)->rsp = SP
#define set_frame_ip(FRAME, IP) (FRAME)->rip = IP
#define get_frame_ip(FRAME) (FRAME)->rip

#endif      /* !defined(__ASSEMBLER__) */
#endif  /* !_AMD64_FRAME_H_ */

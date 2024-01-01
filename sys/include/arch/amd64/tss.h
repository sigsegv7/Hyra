/*
 * Copyright (c) 2024 Ian Marco Moffett and the Osmora Team.
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

#ifndef _AMD64_TSS_H_
#define _AMD64_TSS_H_

#include <sys/types.h>
#include <sys/cdefs.h>

/*
 * cpu_info is from machine/cpu.h
 *
 * XXX: machine/cpu.h includes this header
 *      so we must create this as including
 *      machine/cpu.h will not do anything...
 */
struct cpu_info;

/*
 * A TSS entry (64-bit)
 *
 * See Intel SDM Section 8.2.1 - Task-State Segment (TSS)
 */
struct __packed tss_entry {
    uint32_t reserved1;
    uint32_t rsp0_lo;
    uint32_t rsp0_hi;
    uint32_t rsp1_lo;
    uint32_t rsp1_hi;
    uint32_t rsp2_lo;
    uint32_t rsp2_hi;
    uint64_t reserved2;
    uint32_t ist1_lo;
    uint32_t ist1_hi;
    uint32_t ist2_lo;
    uint32_t ist2_hi;
    uint32_t ist3_lo;
    uint32_t ist3_hi;
    uint32_t ist4_lo;
    uint32_t ist4_hi;
    uint32_t ist5_lo;
    uint32_t ist5_hi;
    uint32_t ist6_lo;
    uint32_t ist6_hi;
    uint32_t ist7_lo;
    uint32_t ist7_hi;
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t io_base;
};

/*
 * TSS descriptor (64-bit)
 *
 * The TSS descriptor describes the location
 * of the TSS segments among other things...
 *
 * See Intel SDM Section 8.2.3 - TSS Descriptor in 64-bit mode
 */
struct __packed tss_desc {
    uint16_t seglimit;
    uint16_t base_lo16;
    uint8_t base_mid8;
    uint8_t type        : 4;
    uint8_t zero        : 1;
    uint8_t dpl         : 2;
    uint8_t p           : 1;
    uint8_t seglimit_hi : 4;
    uint8_t avl         : 1;
    uint8_t unused      : 2;
    uint8_t g           : 1;
    uint8_t base_hi_mid8;
    uint32_t base_hi32;
    uint32_t reserved;
};

/*
 * Holds the address of the address pointing
 * to the top of an interrupt stack.
 */
union tss_stack {
    struct {
        uint32_t top_lo;
        uint32_t top_hi;
    };
    uint64_t top;
};

int tss_alloc_stack(union tss_stack *entry_out, size_t size);
int tss_update_ist(struct cpu_info *ci, union tss_stack stack, uint8_t istno);
void write_tss(struct cpu_info *cpu, struct tss_desc *desc);
void tss_load(void);        /* In tss.S */

#endif  /* _!_AMD64_TSS_H_ */

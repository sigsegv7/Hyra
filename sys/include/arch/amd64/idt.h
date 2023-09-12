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

#ifndef _AMD64_IDT_H_
#define _AMD64_IDT_H_

#include <sys/types.h>
#include <sys/cdefs.h>

#define LIDT(idtr) __ASMV("lidt %0"         \
                          :: "m" (idtr))

#define IDT_TRAP_GATE_FLAGS     0x8F
#define IDT_INT_GATE_FLAGS      0x8E
#define IDT_INT_GATE_USER       0xEE

/*
 * AMD64 Interrupt Gate Descriptor.
 */
struct idt_entry {
    uint16_t    off_lo;         /* Low 16 bits of ISR offset */
    uint16_t    segsel;         /* Segment selector, hardcode to kernel CS */
    uint8_t     ist    : 2;     /* Interrupt stack table */
    uint8_t     zero   : 1;     /* Unused: keep zero */
    uint8_t     zero1  : 4;     /* Unused: keep zero */
    uint8_t     type   : 4;     /* Gate type */
    uint8_t     zero2  : 1;     /* Unused: keep zero */
    uint8_t     dpl    : 2;     /* Descriptor privilege level */
    uint8_t     p      : 1;     /* Present (keep 1 to mark as valid) */
    uint16_t    off_mid;        /* Middle 16 bits of ISR offset */
    uint32_t    off_hi;         /* High 32-bits of ISR offset */
    uint32_t    reserved;       /* Reserved: keep zero */
};

struct __packed idtr {
    uint16_t limit;
    uintptr_t offset;
};


void idt_set_desc(uint8_t vec, uint8_t type, uintptr_t isr, uint8_t ist);
void idt_load(void);

#endif      /* !_AMD64_IDT_H_ */

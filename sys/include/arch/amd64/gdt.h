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

#ifndef AMD64_GDT_H_
#define AMD64_GDT_H_

#include <sys/types.h>
#include <sys/cdefs.h>

#define GDT_TSS 5
#define KERNEL_CS 0x8
#define KERNEL_DS 0x10

struct __packed gdt_entry {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_hi;
};

struct __packed gdtr {
    uint16_t limit;
    uintptr_t offset;
};

static inline void
gdt_load(struct gdtr *gdtr)
{
        __asm("lgdt %0\n"
              "push $8\n"               /* Push CS */
              "lea 1f(%%rip), %%rax\n"  /* Load 1 label address into RAX */
              "push %%rax\n"            /* Push the return address (label 1) */
              "lretq\n"                 /* Far return to update CS */
              "1:\n"
              "  mov $0x10, %%eax\n"
              "  mov %%eax, %%ds\n"
              "  mov %%eax, %%es\n"
              "  mov %%eax, %%fs\n"
              "  mov %%eax, %%gs\n"
              "  mov %%eax, %%ss\n"
              :
              : "m" (*gdtr)
              : "rax", "memory"
        );
}

extern struct gdt_entry g_gdt[256];
extern struct gdt_entry *g_gdt_tss;
extern struct gdtr g_gdtr;

#endif  /* !AMD64_GDT_H_ */

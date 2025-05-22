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

#include <machine/gdt.h>

/*
 * The GDT should be cache line aligned, since it is accessed every time a
 * segment selector is reloaded
 */
__cacheline_aligned struct gdt_entry g_gdt_data[GDT_ENTRY_COUNT] = {
    /* Null */
    {0},

    /* Kernel code (0x08) */
    {
        .limit      = 0x0000,
        .base_low   = 0x0000,
        .base_mid   = 0x00,
        .attributes = GDT_ATTRIBUTE_64BIT_CODE | GDT_ATTRIBUTE_PRESENT   |
                      GDT_ATTRIBUTE_DPL0       | GDT_ATTRIBUTE_NONSYSTEM |
                      GDT_ATTRIBUTE_EXECUTABLE | GDT_ATTRIBUTE_READABLE,
        .base_hi    = 0x00
    },

    /* Kernel data (0x10) */
    {
        .limit      = 0x0000,
        .base_low   = 0x0000,
        .base_mid   = 0x00,
        .attributes = GDT_ATTRIBUTE_PRESENT    | GDT_ATTRIBUTE_DPL0      |
                      GDT_ATTRIBUTE_NONSYSTEM  | GDT_ATTRIBUTE_WRITABLE,
        .base_hi    = 0x00
    },

    /* User code (0x18) */
    {
        .limit      = 0x0000,
        .base_low   = 0x0000,
        .base_mid   = 0x00,
        .attributes = GDT_ATTRIBUTE_64BIT_CODE | GDT_ATTRIBUTE_PRESENT   |
                      GDT_ATTRIBUTE_DPL3       | GDT_ATTRIBUTE_NONSYSTEM |
                      GDT_ATTRIBUTE_EXECUTABLE | GDT_ATTRIBUTE_READABLE,
        .base_hi    = 0x00
    },

    /* User data (0x20) */
    {
        .limit      = 0x0000,
        .base_low   = 0x0000,
        .base_mid   = 0x00,
        .attributes = GDT_ATTRIBUTE_PRESENT    | GDT_ATTRIBUTE_DPL3      |
                      GDT_ATTRIBUTE_NONSYSTEM  | GDT_ATTRIBUTE_WRITABLE,
        .base_hi    = 0x00
    },

    /*
     * TSS segment (0x28)
     *
     * NOTE: 64-bit TSS descriptors are 16 bytes, equivalent to the size of two
     *       regular descriptor entries.
     *       See Intel SPG 3/25 Section 9.2.3 - TSS Descriptor in 64-bit mode.
     */
    {0}, {0}
};

/* Verify that the GDT is of the correct size */
__static_assert(sizeof(g_gdt_data) == (8 * GDT_ENTRY_COUNT));

const struct gdtr g_gdtr = {
    .limit = sizeof(g_gdt_data) - 1,
    .offset = (uintptr_t)&g_gdt_data[0]
};

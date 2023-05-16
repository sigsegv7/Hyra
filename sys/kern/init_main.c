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

#include <sys/cdefs.h>
#include <sys/printk.h>
#include <vt/vt.h>

#if defined(__x86_64__)
# include <arch/x86/cpu.h>
# include <arch/x86/idt.h>
# include <arch/x86/exceptions.h>
# include <arch/x86/gdt.h>
#elif defined(__aarch64__)
# include <arch/aarch64/exception.h>
# include <arch/aarch64/board.h>
#endif

#define COPYRIGHT "Copyright (c) 2023 Ian Marco Moffett and the VegaOS team."

struct vt_descriptor g_vt;

static void
early_cpu_init(void)
{
#if defined(__x86_64__)
    if (amd64_enable_sse() != 0) {
        printk("CPU does not support SSE!\n");
    }

    if (amd64_enable_avx() != 0) {
        printk("CPU does not support AVX!\n");
    }

    idt_load();
    gdt_load(&g_early_gdtr);
#elif defined(__aarch64__)
    printk("Detected board: %s\n", aarch64_get_board());
#endif
    exceptions_init();
}

static void
early_init(void)
{
    vt_init(&g_vt, NULL, NULL);

    printk("-- Vega v%s --\n", VEGA_VERSION);
    printk("%s\n", COPYRIGHT);

    early_cpu_init();
}

__dead void
main(void)
{
    early_init();
    for (;;);
}

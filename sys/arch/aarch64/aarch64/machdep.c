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

#include <sys/syslog.h>
#include <sys/panic.h>
#include <machine/cpu.h>
#include <machine/sync.h>

struct cpu_info g_bsp_ci = {0};

void
cpu_startup(struct cpu_info *ci)
{
    /* TODO: STUB */
    return;
}

void
cpu_halt_others(void)
{
    /* TODO: STUB */
    return;
}

void
serial_init(void)
{
    /* TODO: STUB */
    return;
}

void
md_backtrace(void)
{
    /* TODO: STUB */
    return;
}

void
serial_putc(char c)
{
    /* TODO: STUB */
    return;
}

int
md_sync_all(void)
{
    /* TODO: STUB */
    return 0;
}

void
cpu_halt_all(void)
{
    /* TODO: Stub */
    for (;;);
}

/*
 * Get the descriptor for the currently
 * running processor.
 */
struct cpu_info *
this_cpu(void)
{
    struct cpu_info *ci;

    __ASMV("mrs %0, tpidr_el0" : "=r" (ci));
    return ci;
}

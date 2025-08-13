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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/driver.h>
#include <sys/syslog.h>
#include <machine/tsc.h>
#include <machine/asm.h>

/* See kconf(9) */
#if defined(__USER_TSC)
#define USER_TSC __USER_TSC
#else
#define USER_TSC 0
#endif  /* __USER_TSC */

#define pr_trace(fmt, ...) kprintf("tsc: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static uint64_t tsc_i = 0;

uint64_t
rdtsc_rel(void)
{
    return rdtsc() - tsc_i;
}

static int
tsc_init(void)
{
    uint64_t cr4;

    cr4 = amd64_read_cr4();
    tsc_i = rdtsc();
    pr_trace("initial count @ %d\n", rdtsc_rel());

    /*
     * If we USER_TSC is configured to "yes" then
     * we'll need to enable the 'rdtsc' instruction
     * in user mode.
     */
    if (!USER_TSC) {
        cr4 &= ~CR4_TSD;
    } else {
        cr4 |= CR4_TSD;
    }

    amd64_write_cr4(cr4);
    return 0;
}

DRIVER_EXPORT(tsc_init, "x86-tsc");

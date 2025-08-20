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

#include <sys/reboot.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <machine/pio.h>
#include <machine/cpu.h>
#include <dev/acpi/acpi.h>

static void
cpu_reset_intel(struct cpu_info *ci)
{
    /*
     * Ivy bridge processors and their panther point chipsets
     * (family 6) can be reset through special PCH reset control
     * registers
     */
    if (ci->family == 6) {
        outb(0xCF9, 3 << 1);
    }
}

/*
 * Attempt to reboot the system, we do this in many
 * stages of escalation. If a reset via the i8042
 * controller fails and we are on an Intel processor,
 * attempt a chipset specific reset. If that somehow fails
 * as well, just smack the cpu with a NULL IDTR as well
 * as an INT $0x0
 */
static void
__cpu_reset(struct cpu_info *ci)
{
    /* Try via the i8042 */
    outb(0x64, 0xFE);

    /* Something went wrong if we are here */
    if (ci == NULL) {
        return;
    }

    if (ci->vendor == CPU_VENDOR_INTEL) {
        cpu_reset_intel(ci);
    }
}

void
cpu_reboot(int method)
{
    struct cpu_info *ci = this_cpu();
    uint32_t *__dmmy = NULL;

    if (ISSET(method, REBOOT_POWEROFF)) {
        acpi_sleep(ACPI_SLEEP_S5);
    }

    if (ISSET(method, REBOOT_HALT)) {
        cpu_halt_all();
    }

    __cpu_reset(ci);
    asm volatile("lgdt %0; int $0x0" :: "m" (__dmmy));
    __builtin_unreachable();
}

/*
 * arg0: Method bits
 */
scret_t
sys_reboot(struct syscall_args *scargs)
{
    cpu_reboot(scargs->arg0);
    __builtin_unreachable();
}

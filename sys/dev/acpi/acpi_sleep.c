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

#include <sys/errno.h>
#include <sys/syslog.h>
#include <machine/cdefs.h>
#include <machine/cpu.h>
#include <dev/acpi/acpi.h>
#include <uacpi/sleep.h>

#define pr_trace(fmt, ...) kprintf("acpi: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

int
acpi_sleep(int type)
{
    uacpi_status error;
    uacpi_sleep_state state;
    const uacpi_char *error_str;

    switch (type) {
    case ACPI_SLEEP_S5:
        state = UACPI_SLEEP_STATE_S5;
        break;
    default:
        return -EINVAL;
    }

    error = uacpi_prepare_for_sleep_state(state);
    if (uacpi_unlikely_error(error)) {
        error_str = uacpi_status_to_string(error);
        pr_error("failed to prep sleep: %s\n", error_str);
        return -EIO;
    }

    /*
     * If we are entering the S5 sleep state, bring
     * everything down first.
     */
    if (type == ACPI_SLEEP_S5) {
        pr_trace("powering off, halting all cores...\n");
        cpu_halt_others();
        md_intoff();
    }

    error = uacpi_enter_sleep_state(UACPI_SLEEP_STATE_S5);
    if (uacpi_unlikely_error(error)) {
        error_str = uacpi_status_to_string(error);
        pr_error("could not enter S5 state: %s\n", error_str);
        return -EIO;
    }

    return 0;
}

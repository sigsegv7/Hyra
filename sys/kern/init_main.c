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

#include <sys/reboot.h>
#include <sys/syslog.h>
#include <sys/sched.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <dev/cons/cons.h>
#include <dev/acpi/acpi.h>
#include <machine/cpu.h>
#include <vm/vm.h>

static struct proc proc0;

int
main(void)
{
    /* Startup the console */
    cons_init();
    kprintf("Starting Hyra/%s v%s: %s\n", HYRA_ARCH, HYRA_VERSION,
        HYRA_BUILDDATE);

    /* Start the ACPI subsystem */
    acpi_init();

    /* Init the virtual memory subsystem */
    vm_init();

    /* Startup the BSP */
    cpu_startup(&g_bsp_ci);

    /* Init process 0 */
    md_td_init(&proc0, NULL, 0);

    /* Init the virtual file system */
    vfs_init();

    /* Start scheduler and bootstrap APs */
    sched_init();
    mp_bootstrap_aps(&g_bsp_ci);

    /* Nothing left to do... halt */
    cpu_reboot(REBOOT_HALT);
    __builtin_unreachable();
}

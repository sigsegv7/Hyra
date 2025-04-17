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
#include <sys/syslog.h>
#include <sys/sched.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/driver.h>
#include <sys/panic.h>
#include <dev/cons/cons.h>
#include <dev/acpi/acpi.h>
#include <machine/cpu.h>
#include <machine/cdefs.h>
#include <vm/vm.h>
#include <string.h>

static struct proc proc0;

static void
copyright(void)
{
    kprintf(OMIT_TIMESTAMP
           "Copyright (c) 2023-2025 Ian Marco Moffett and the OSMORA team\n");
}

static void
start_init(void)
{
#if 0
    struct proc *td = this_td();
    struct execve_args execve_args;
    char *argv[] = { "/usr/sbin/init", NULL };
    char *envp[] = { NULL };

    execve_args.pathname = argv[0];
    execve_args.argv = argv;
    execve_args.envp = envp;
    if (execve(td, &execve_args) != 0)
        panic("failed to load init\n");
#endif
    for (;;);
}

int
main(void)
{
    /* Setup serial driver */
    serial_init();

    /* Startup the console */
    cons_init();
    copyright();
    kprintf("Starting Hyra/%s v%s: %s\n", HYRA_ARCH, HYRA_VERSION,
        HYRA_BUILDDATE);

    /* Init the virtual memory subsystem */
    vm_init();

    /* Start the ACPI subsystem */
    acpi_init();

    /* Startup the BSP */
    cpu_startup(&g_bsp_ci);

    /* Init the virtual file system */
    vfs_init();

    /* Expose the console to devfs */
    cons_expose();

    /* Start scheduler and bootstrap APs */
    md_intoff();
    sched_init();

    /* Startup pid 1 */
    memset(&proc0, 0, sizeof(proc0.tf));
    fork1(&proc0, 0, start_init, NULL);

    /* Load all drivers */
    DRIVERS_INIT();

    /* Bootstrap APs and here we go! */
    mp_bootstrap_aps(&g_bsp_ci);
    sched_enter();
    __builtin_unreachable();
}

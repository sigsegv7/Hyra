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
#include <sys/systm.h>
#include <dev/acpi/uacpi.h>
#include <dev/cons/cons.h>
#include <dev/acpi/acpi.h>
#include <machine/cpu.h>
#include <machine/cdefs.h>
#include <vm/vm.h>
#include <string.h>

#define _START_PATH "/usr/sbin/init"
#if defined(_INSTALL_MEDIA)
#define _START_ARG "/usr/sbin/install"
#else
#define _START_ARG NULL
#endif  /* _INSTALL_MEDIA  */

struct proc g_proc0;
struct proc *g_init;

static void
copyright(void)
{
    kprintf(OMIT_TIMESTAMP
           "Copyright (c) 2023-2025 Ian Marco Moffett and the OSMORA team\n");
}

static void
start_init(void)
{
    struct proc *td = this_td();
    struct execve_args execve_args;
    char *argv[] = { _START_PATH, _START_ARG, NULL };
    char *envp[] = { NULL };

    kprintf("starting init...\n");
    execve_args.pathname = argv[0];
    execve_args.argv = argv;
    execve_args.envp = envp;
    if (execve(td, &execve_args) != 0)
        panic("failed to load init\n");

    __builtin_unreachable();
}

int
main(void)
{
    /* Setup serial driver */
    serial_init();

    /* Init the virtual memory subsystem */
    vm_init();

    /* Startup the console */
    cons_init();
    copyright();
    kprintf("Starting Hyra/%s v%s: %s\n", HYRA_ARCH, HYRA_VERSION,
        HYRA_BUILDDATE);

    /* Start the ACPI subsystem */
    acpi_init();

    /* Startup the BSP */
    cpu_startup(&g_bsp_ci);

    /* Init the virtual file system */
    vfs_init();

    /* Expose the console to devfs */
    cons_expose();

    uacpi_init();

    /* Start scheduler and bootstrap APs */
    md_intoff();
    sched_init();

    memset(&g_proc0, 0, sizeof(g_proc0));

    /* Startup pid 1 */
    spawn(&g_proc0, start_init, NULL, 0, &g_init);
    md_inton();

    /* Load all early drivers */
    DRIVERS_INIT();

    /* Only log to kmsg from here */
    syslog_silence(true);

    /*
     * Bootstrap APs, schedule all other drivers
     * and here we go!
     */
    mp_bootstrap_aps(&g_bsp_ci);
    sched_enter();
    __builtin_unreachable();
}

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

#include <sys/cdefs.h>
#include <sys/tty.h>
#include <sys/syslog.h>
#include <sys/machdep.h>
#include <sys/timer.h>
#include <firmware/acpi/acpi.h>
#include <vm/physseg.h>
#include <vm/vm.h>
#include <logo.h>

__MODULE_NAME("init_main");
__KERNEL_META("$Hyra$: init_main.c, Ian Marco Moffett, "
              "Where the Hyra kernel first starts up");

static inline void
log_timer(const char *purpose, tmrr_status_t s, const struct timer *tmr)
{
    if (s == TMRR_EMPTY_ENTRY) {
        KINFO("%s not yet registered\n", purpose);
    } else if (tmr->name == NULL) {
        KINFO("Nameless %s registered; unknown\n", purpose);
    } else {
        KINFO("%s registered: %s\n", purpose, tmr->name);
    }
}

/*
 * Logs what timers are registered
 * on the system.
 */
static void
list_timers(void)
{
    struct timer timer_tmp;
    tmrr_status_t status;

    status = req_timer(TIMER_SCHED, &timer_tmp);
    log_timer("SCHED_TMR", status, &timer_tmp);

    status = req_timer(TIMER_GP, &timer_tmp);
    log_timer("GENERAL_PURPOSE_TMR", status, &timer_tmp);
}

void
main(void)
{
    tty_init();
    syslog_init();
    PRINT_LOGO();

    kprintf("Hyra/%s v%s: %s (%s)\n",
            HYRA_ARCH, HYRA_VERSION, HYRA_BUILDDATE,
            HYRA_BUILDBRANCH);

    acpi_init();

    /*
     * TODO: Move these calls to machdep.c whenever
     *       possible. It must be documented that
     *       this will only be called by processor_init()
     *       as the pmap subsystem may rely on architecture
     *       specifics that haven't been set up yet... Putting
     *       these calls in processor_init() makes things more
     *       flexible.
     */
    vm_physseg_init();
    vm_init();

    processor_init();
    list_timers();

    /* We're done here, halt the processor */
    __ASMV("cli; hlt");
}

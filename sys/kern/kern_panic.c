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

#include <sys/panic.h>
#include <sys/spinlock.h>
#include <sys/syslog.h>
#include <sys/reboot.h>
#include <dev/cons/cons.h>
#include <machine/cdefs.h>
#include <machine/cpu.h>
#include <string.h>

#if defined(__PANIC_SCR)
#define PANIC_SCR  __PANIC_SCR
#else
#define PANIC_SCR 0
#endif

static void
panic_puts(const char *str)
{
    size_t len;

    len = strlen(str);
    cons_putstr(&g_root_scr, str, len);
}

/*
 * Burn and sizzle - the core logic that really ends
 * things ::)
 *
 * @do_trace: If true, a backtrace will be printed
 * @reboot_type: REBOOT_* defines
 */
static void
bas(bool do_trace, int reboot_type)
{
    static struct spinlock lock = {0};

    spinlock_acquire(&lock);    /* Never released */

    if (do_trace) {
        panic_puts(" ** backtrace\n");
        md_backtrace();
    }

    panic_puts("\n-- ALL CORES HAVE BEEN HALTED --\n");
    cpu_reboot(reboot_type);
    __builtin_unreachable();
}

static void
panic_screen(void)
{
    struct cons_screen *scr = &g_root_scr;

    if (scr->fb_mem != NULL) {
        scr->bg = 0x8B0000;
        scr->fg = 0xAABBAA;
        cons_reset_cursor(scr);
        cons_clear_scr(scr, 0x393B39);
    }
}

static void
do_panic(const char *fmt, va_list *ap)
{
    syslog_silence(false);
    spinlock_release(&g_root_scr.lock);
    panic_puts("panic: ");
    vkprintf(fmt, ap);
    bas(true, REBOOT_HALT);

    __builtin_unreachable();
}

/*
 * Tells the user something terribly wrong happened then
 * halting the system as soon as possible.
 *
 * XXX: There is no need to cleanup stuff here (e.g `va_list ap`)
 *      as we won't be returning from here anyways and the source
 *      of the panic could be *anywhere* so it's best not to mess with
 *      things.
 */
void
panic(const char *fmt, ...)
{
    va_list ap;

    /* Shut everything else up */
    md_intoff();
    cpu_halt_others();

    if (PANIC_SCR) {
        panic_screen();
    }
    va_start(ap, fmt);
    do_panic(fmt, &ap);
    __builtin_unreachable();
}

/*
 * Halt and catch fire - immediately ceases all system activity
 * with an optional message.
 *
 * @fmt: printf() format string, NULL to not
 *       specify any message (not recommended)
 */
void
hcf(const char *fmt, ...)
{
    va_list ap;

    if (fmt != NULL) {
        va_start(ap, fmt);
        kprintf(OMIT_TIMESTAMP);
        vkprintf(fmt, &ap);
    }

    bas(true, REBOOT_HALT);
    __builtin_unreachable();
}

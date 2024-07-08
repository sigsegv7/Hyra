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

#include <sys/syslog.h>
#include <sys/spinlock.h>
#include <dev/cons/cons.h>
#include <dev/timer.h>
#include <stdarg.h>
#include <string.h>

#if defined(__SERIAL_DEBUG)
#define SERIAL_DEBUG __SERIAL_DEBUG
#else
#define SERIAL_DEBUG 0
#endif

/* Global logger lock */
static struct spinlock lock = {0};

static void
syslog_write(const char *s, size_t len)
{
    const char *p = s;

    while (len--) {
        cons_putch(&g_root_scr, *p);
        if (SERIAL_DEBUG) {
            serial_putc(*p);
        }
        ++p;
    }
}

/*
 * XXX: Not serialized
 */
void
vkprintf(const char *fmt, va_list *ap)
{
    char buffer[1024] = {0};

    vsnprintf(buffer, sizeof(buffer), fmt, *ap);
    syslog_write(buffer, strlen(buffer));
}

void
kprintf(const char *fmt, ...)
{
    va_list ap;
    char timestamp[64] = "[  0.000000] ";
    const char *fmt_p = fmt;
    bool use_timestamp = true;
    bool has_counter = true;
    struct timer tmr;

    /*
     * If the first char is OMIT_TIMESTAMP, then we won't
     * print out the timestamp.
     */
    if (*fmt == OMIT_TIMESTAMP[0]) {
        ++fmt_p;
        use_timestamp = false;
    }

    /* See if we have a timer */
    if (req_timer(TIMER_GP, &tmr) != TMRR_SUCCESS) {
        has_counter = false;
    }

    /* If we can use the counter, format the timestamp */
    if (has_counter) {
        if (tmr.get_time_sec != NULL && tmr.get_time_usec != NULL) {
            snprintf(timestamp, sizeof(timestamp), "[  %d.%06d] ",
                tmr.get_time_sec(), tmr.get_time_usec());
        }
    }

    if (use_timestamp) {
        syslog_write(timestamp, strlen(timestamp));
    }

    spinlock_acquire(&lock);
    va_start(ap, fmt);

    vkprintf(fmt_p, &ap);
    va_end(ap);
    spinlock_release(&lock);
}

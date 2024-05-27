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
#include <sys/machdep.h>
#include <sys/tty.h>
#include <sys/cdefs.h>
#include <sys/timer.h>
#include <dev/vcons/vcons.h>
#include <fs/procfs.h>
#include <string.h>

#if defined(__KMSG_BUF_SHIFT)
#define KMSG_BUF_SHIFT __KMSG_BUF_SHIFT
#else
#define KMSG_BUF_SHIFT 12
#endif

#define KMSG_BUF_SIZE (1 << KMSG_BUF_SHIFT)

__STATIC_ASSERT(KMSG_BUF_SHIFT <= 16, "Log buffer shift too large!\n");

static char kmsg_buf[KMSG_BUF_SIZE];
static size_t kmsg_buf_idx = 0;
static struct proc_entry *kmsg_proc;

struct vcons_screen g_syslog_screen = {0};
bool g_syslog_use_tty = true;

static inline void
kmsg_buf_putc(char c)
{
    kmsg_buf[kmsg_buf_idx++] = c;
    kmsg_buf[kmsg_buf_idx] = '\0';
    if (kmsg_buf_idx >= (KMSG_BUF_SIZE - 1))
        kmsg_buf_idx = 0;
}

static int
proc_kmsg_read(struct proc_entry *p, struct sio_txn *sio)
{
    if (sio->len > KMSG_BUF_SIZE)
        sio->len = KMSG_BUF_SIZE;

    memcpy(sio->buf, kmsg_buf, sio->len);
    return sio->len;
}

static void
syslog_write(const char *s, size_t len)
{
    size_t tmp_len = len;
    const char *tmp_s = s;

    while (tmp_len--) {
#if defined(__SERIAL_DEBUG)
        serial_dbgch(*tmp_s);
#endif  /* defined(__SERIAL_DEBUG) */
        kmsg_buf_putc(*tmp_s);
        if (g_syslog_use_tty)
            tty_putc(&g_root_tty, *tmp_s, TTY_SOURCE_RAW);

        ++tmp_s;
    }

    tty_flush(&g_root_tty);
}

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
    bool has_counter = true;
    bool use_timestamp = true;
    const char *fmt_p = fmt;
    struct timer tmr = {0};

    /*
     * If the first char is OMIT_TIMESTAMP, than we won't
     * print out the timestamp.
     */
    if (*fmt_p == OMIT_TIMESTAMP[0]) {
        ++fmt_p;
        use_timestamp = false;
    }

    /* See if we can use the counter */
    if (req_timer(TIMER_GP, &tmr) != 0)
        has_counter = false;

    if (has_counter) {
        if (tmr.get_time_sec != NULL && tmr.get_time_usec != NULL)
            snprintf(timestamp, sizeof(timestamp), "[  %d.%06d] ",
                     tmr.get_time_sec(), tmr.get_time_usec());

    }

    if (use_timestamp) {
        syslog_write(timestamp, strlen(timestamp));
    }

    va_start(ap, fmt);
    vkprintf(fmt_p, &ap);
    va_end(ap);
}

void
syslog_init_proc(void)
{
    kmsg_proc = procfs_alloc_entry();
    kmsg_proc->read = proc_kmsg_read;
    procfs_add_entry("kmsg", kmsg_proc);
}

void
syslog_init(void)
{
    g_syslog_screen.bg = 0x000000;
    g_syslog_screen.fg = 0x808080;

    vcons_attach(&g_syslog_screen);
}

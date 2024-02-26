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
#include <dev/vcons/vcons.h>
#include <string.h>

static struct vcons_screen syslog_screen = {0};

static void
syslog_write(const char *s, size_t len)
{
    size_t tmp_len = len;
    const char *tmp_s = s;

    while (tmp_len--) {
#if defined(__SERIAL_DEBUG)
        serial_dbgch(*tmp_s);
#endif  /* defined(__SERIAL_DEBUG) */
        vcons_putch(&syslog_screen, *tmp_s++);
    }
}

/*
 * TODO: Replace with vsnprintf()
 */
static void
syslog_handle_fmt(va_list *ap, char fmt_spec)
{
    char tmp_ch;
    int64_t tmp_int;
    const char *tmp_str;
    char tmp_buf[256] = { 0 };

    size_t tmp_len;

    switch (fmt_spec) {
    case 'c':
        tmp_ch = va_arg(*ap, int);
        syslog_write(&tmp_ch, 1);
        break;
    case 's':
        tmp_str = va_arg(*ap, const char *);
        syslog_write(tmp_str, strlen(tmp_str));
        break;
    case 'd':
        tmp_int = va_arg(*ap, int64_t);
        itoa(tmp_int, tmp_buf, 10);
        syslog_write(tmp_buf, strlen(tmp_buf));
        break;
    case 'p':
        tmp_int = va_arg(*ap, uint64_t);
        itoa(tmp_int, tmp_buf, 16);
        tmp_len = strlen(tmp_buf);
        /*
         * Now we pad this.
         *
         * XXX TODO: This assumes 64-bits, should be
         *           cleaned up.
         */
        for (size_t i = 0; i < 18 - tmp_len; ++i) {
            syslog_write("0", 1);
        }
        syslog_write(tmp_buf + 2, tmp_len - 2);
        break;
    case 'x':
        tmp_int = va_arg(*ap, uint64_t);
        itoa(tmp_int, tmp_buf, 16);
        tmp_len = strlen(tmp_buf);
        syslog_write(tmp_buf + 2, tmp_len - 2);
        break;
    }
}

void
vkprintf(const char *fmt, va_list *ap)
{
    while (*fmt) {
        if (*fmt == '%') {
            ++fmt;
            syslog_handle_fmt(ap, *fmt++);
        } else {
            syslog_write(fmt++, 1);
        }
    }
}

void
kprintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vkprintf(fmt, &ap);
    va_end(ap);
}

void
syslog_init(void)
{
    struct termios termios = {0};

    termios.c_oflag |= OCRNL;   /* Map CR to NL by default */

    syslog_screen.bg = 0x000000;
    syslog_screen.fg = 0x808080;
    syslog_screen.termios = termios;

    vcons_attach(&syslog_screen);
}

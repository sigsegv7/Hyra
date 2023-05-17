/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

#include <sys/printk.h>
#include <vt/vt.h>
#include <string.h>

extern struct vt_descriptor g_vt;

static void
printk_handle_format(char fmt_code, va_list *ap)
{
    char buf[80];

    int64_t tmp;
    char c;
    const char *s;

    switch (fmt_code) {
    case 'c':
        c = va_arg(*ap, int);
        vt_write(&g_vt, &c, 1);
        break;
    case 's':
        s = va_arg(*ap, const char *);
        vt_write(&g_vt, s, strlen(s));
        break;
    case 'p':
    case 'x':
        tmp = va_arg(*ap, int64_t);
        s = itoa(tmp, buf, 16) + 2;     /* + 2 to skip "0x" prefix */
        vt_write(&g_vt, s, strlen(s));
        break;
    case 'd':
        tmp = va_arg(*ap, int64_t);
        s = itoa(tmp, buf, 10);     /* + 2 to skip "0x" prefix */
        vt_write(&g_vt, s, strlen(s));
    }
}

/*
 * NOTE: The `va_list` pointer is a workaround
 *       for a quirk in AARCH64 for functions
 *       with variable arguments.
 */

void
vprintk(const char *fmt, va_list *ap)
{
    while (*fmt) {
        if (*fmt == '%') {
            ++fmt;
            printk_handle_format(*fmt, ap);
        } else {
            vt_write(&g_vt, fmt, 1);
        }

        ++fmt;
    }
}

void
printk(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    vprintk(fmt, &ap);

    va_end(ap);
}

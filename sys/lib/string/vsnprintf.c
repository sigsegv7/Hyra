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

#include <sys/types.h>
#include <string.h>
#include <stdarg.h>

static inline void
printc(char *buf, size_t size, size_t *off, char c)
{
    if (*off < size - 1) {
        buf[(*off)++] = c;
    }
    buf[*off] = 0;
}

static void
printstr(char *buf, size_t size, size_t *off, const char *s)
{
    while (*off < size - 1 && *s != '\0') {
        buf[(*off)++] = *(s)++;
    }
    buf[*off] = 0;
}

int
snprintf(char *s, size_t size, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vsnprintf(s, size, fmt, ap);
    va_end(ap);
    return ret;
}

int
vsnprintf(char *s, size_t size, const char *fmt, va_list ap)
{
    size_t tmp_len, num_len, off = 0;
    ssize_t num = 0;
    char c, c1, num_buf[256] = {0};
    const char *tmp_str;
    uint8_t pad_width = 0;

    while (off < (size - 1)) {
        while (*fmt && *fmt != '%') {
            printc(s, size, &off, *fmt++);
        }
        if (*(fmt)++ == '\0' || off == size - 1) {
            return off;
        }

        /*
         * Handle a case where we have "%0N". For example:
         * "%04d"
         */
        if (*fmt == '0') {
            ++fmt;
            while (*fmt >= '0' && *fmt <= '9') {
                pad_width = pad_width * 10 + (*fmt - '0');
                ++fmt;
            }
        }

        c = *fmt++;
        switch (c) {
        case 'c':
            c1 = (char )va_arg(ap, int);
            printc(s, size, &off, c1);
            break;
        case 'd':
            num = va_arg(ap, int);
            itoa(num, num_buf, 10);

            if (pad_width > 0) {
                num_len = strlen(num_buf);
                for (size_t i = num_len; i < pad_width; ++i)
                    printc(s, size, &off, '0');
            }
            printstr(s, size, &off, num_buf);
            break;
        case 'p':
            num = va_arg(ap, uint64_t);
            itoa(num, num_buf, 16);
            tmp_len = strlen(num_buf);

            /* Add '0x' prefix */
            printc(s, size, &off, '0');
            printc(s, size, &off, 'x');
            /*
             * Now we pad this.
             *
             * XXX TODO: This assumes 64-bits, should be
             *           cleaned up.
             */
            for (size_t i = 0; i < 18 - tmp_len; ++i) {
                printc(s, size, &off, '0');
            }
            printstr(s, size, &off, num_buf + 2);
            break;
        case 'x':
            num = va_arg(ap, uint64_t);
            itoa(num, num_buf, 16);
            tmp_len = strlen(num_buf);
            printstr(s, size, &off, num_buf + 2);
            break;
        case 's':
            tmp_str = va_arg(ap, const char *);
            printstr(s, size, &off, tmp_str);
            break;
        }
    }

    return 0;
}

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
#include <stdint.h>
#include <stdio.h>

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

static void
dec_to_str(int value, char *buf, size_t size, size_t *off)
{
        size_t i = 0;
        uint8_t is_negative = 0;
        uint8_t tmp;
        char intbuf[4096];

        if (value == 0) {
            intbuf[i++] = '0';
            intbuf[i++] = '\0';
            printstr(buf, size, off, intbuf);
            return;
        }

        if (value < 0) {
            /* Easier to handle positive numbers */
            value *= -1;
            is_negative = 1;
        }

        while (value > 0) {
            intbuf[i++] = '0' + (value % 10);
            value /= 10;
        }

        if (is_negative) {
            intbuf[i++] = '-';
        }

        intbuf[i--] = '\0';

        /* Result is in reverse */
        for (int j = 0; j < i; ++j, --i) {
            tmp = intbuf[j];
            intbuf[j] = intbuf[i];
            intbuf[i] = tmp;
        }

        printstr(buf, size, off, intbuf);
}

int
vsnprintf(char *s, size_t size, const char *fmt, va_list ap)
{
    size_t tmp_len, off = 0;
    ssize_t num = 0;
    char c, c1;
    const char *tmp_str;
    int tmp_num;

    while (off < (size - 1)) {
        while (*fmt && *fmt != '%') {
            printc(s, size, &off, *fmt++);
        }

        if (*(fmt)++ == '\0' || off == size - 1) {
            return off;
        }

        c = *fmt++;
        switch (c) {
        case 'c':
            c1 = (char)va_arg(ap, int);
            printc(s, size, &off, c1);
            break;
        case 's':
            tmp_str = va_arg(ap, const char *);
            printstr(s, size, &off, tmp_str);
            break;
        case 'd':
            tmp_num = va_arg(ap, int);
            dec_to_str(tmp_num, s, size, &off);
            break;
        }
    }

    return 0;
}

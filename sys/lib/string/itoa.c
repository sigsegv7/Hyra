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

#include <string.h>

static char *itoa_base10_convert(int64_t value, char *buf);

static char *
itoa_base10_convert(int64_t value, char *buf)
{
        size_t i;
        uint8_t tmp;
        bool is_negative;

        i = 0;
        is_negative = false;

        if (value == 0) {
            buf[i++] = '0';
            buf[i++] = '\0';
            return buf;
        }

        if (value < 0) {
            /* Easier to handle positive numbers */
            value *= -1;
            is_negative = true;
        }

        while (value > 0) {
            buf[i++] = '0' + (value % 10);
            value /= 10;
        }

        if (is_negative) {
            buf[i++] = '-';
        }

        buf[i--] = '\0';

        /* Result is in reverse */
        for (int j = 0; j < i; ++j, --i) {
            tmp = buf[j];
            buf[j] = buf[i];
            buf[i] = tmp;
        }

        return buf;
}

static char *
itoa_convert_base16(uint64_t n, char *buffer)
{
    bool pad;
    uint8_t nibble;
    uint8_t i, j, tmp;
    const char *ascii_nums = "0123456789ABCDEF";

    i = 0;
    pad = false;

    if (n == 0) {
            /* Zero, no need to parse */
            memcpy(buffer, "0x00\0", 5);
            return buffer;
    }
    /* If one digit, pad out 2 later */
    if (n < 0x10) {
            pad = true;
    }

    while (n > 0) {
            nibble = (uint8_t)n & 0x0F;
            nibble = ascii_nums[nibble];
            buffer[i++] = nibble;
            n >>= 4;        /* Fetch next nibble */
    }

    if (pad) {
            buffer[i++] = '0';
    }

    /* Add "0x" prefix */
    buffer[i++] = 'x';
    buffer[i++] = '0';
    buffer[i--] = '\0';

    /* Unreverse the result */
    for (j = 0; j < i; ++j, --i) {
            tmp = buffer[j];
            buffer[j] = buffer[i];
            buffer[i] = tmp;
    }
    return buffer;
}

char *
itoa(int64_t value, char *buf, int base)
{
    switch (base) {
        case 10:
            return itoa_base10_convert(value, buf);
        case 16:
            return itoa_convert_base16(value, buf);
        default:
            return NULL;
    }
}

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

static char *
itoa_convert_base10(int64_t n, char *buffer)
{
    uint8_t i = 0, j, tmp;
    bool is_negative = false;

    if (n == 0) {
        /* No need to parse if number is zero */
        buffer[i++] = '0';
        buffer[i++] = '\0';
        return buffer;
    }

    if (n < 0) {
        /* Positive numbers are easier to work with */
        n *= -1;
        is_negative = true;
    }

    while (n > 0) {
        buffer[i++] = (n % 10) + '0';
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i--] = '\0';

    /* Result is reserved, so fix that up */
    for (j = 0; j < i; ++j, --i) {
        tmp = buffer[j];
        buffer[j] = buffer[i];
        buffer[i] = tmp;
    }

    return buffer;
}

static char *
itoa_convert_base16(uint64_t n, char *buffer)
{
    const char *ascii_nums = "0123456789ABCDEF";
    bool pad;
    uint8_t nibble;
    uint8_t i = 0, j, tmp;

    if (n == 0) {
        /* Zero, no need to parse */
        memcpy(buffer, "0x00\n", 5);
        return buffer;
    }

    /* If one digit, pad out to 2 later */
    if (n < 0x10) {
        pad = 1;
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

    /* Unreverse the number */
    for (j = 0; j < i; ++j, --i) {
        tmp = buffer[j];
        buffer[j] = buffer[i];
        buffer[i] = tmp;
    }

    return buffer;
}

char *
itoa(int64_t n, char *buf, int radix)
{
    switch (radix) {
    case 10:
        return itoa_convert_base10(n, buf);
    case 16:
        return itoa_convert_base16(n, buf);
    }

    return NULL;
}

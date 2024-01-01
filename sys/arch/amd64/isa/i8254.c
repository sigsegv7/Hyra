/*
 * Copyright (c) 2024 Ian Marco Moffett and the Osmora Team.
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

#include <machine/isa/i8254.h>
#include <machine/io.h>
#include <sys/types.h>
#include <sys/cdefs.h>

/*
 * Fetches the current count.
 */
uint16_t
i8254_get_count(void)
{
    uint8_t lo, hi;

    outb(i8254_COMMAND, 0x00);
    lo = inb(0x40);
    hi = inb(0x40);
    return __COMBINE8(hi, lo);
}

/*
 * Set the reload value
 *
 * The reload value is where the i8254's counter
 * starts...
 */
void
i8254_set_reload(uint16_t val)
{
    /* Channel 0, lo/hi access, rate generator */
    outb(i8254_COMMAND, 0x34);

    outb(0x40, (val & 0xFF));
    outb(0x40, (val >> 8) & 0xFF);
}

void
i8254_set_frequency(uint64_t freq_hz)
{
    uint64_t divisor = i8254_DIVIDEND / freq_hz;

    if ((i8254_DIVIDEND % freq_hz) > (freq_hz / 2)) {
        ++divisor;
    }

    i8254_set_reload(freq_hz);
}

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

#include <sys/cdefs.h>
#include <sys/timer.h>
#include <sys/errno.h>
#include <machine/isa/spkr.h>
#include <machine/isa/i8254.h>
#include <machine/io.h>

#define DIVIDEND 1193180
#define CTRL_PORT 0x61

int
pcspkr_tone(uint16_t freq, uint32_t msec)
{
    uint32_t divisor;
    uint8_t tmp;
    struct timer tmr;

    if (req_timer(TIMER_GP, &tmr) != TMRR_SUCCESS)
        return -ENOTSUP;
    if (__unlikely(tmr.msleep == NULL))
        return -ENOTSUP;

    divisor = DIVIDEND / freq;
    outb(i8254_COMMAND, 0xB6);
    outb(i8254_CHANNEL_2, divisor & 0xFF);
    outb(i8254_CHANNEL_2, (divisor >> 8) & 0xFF);

    /* Oscillate the speaker */
    tmp = inb(CTRL_PORT);
    if (!__TEST(tmp, 3)) {
        tmp |= 3;
        outb(CTRL_PORT, tmp);
    }

    /* Sleep then turn off the speaker */
    tmr.msleep(msec);
    outb(CTRL_PORT, tmp & ~3);
    return 0;
}

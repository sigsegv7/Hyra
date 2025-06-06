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
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/driver.h>
#include <fs/devfs.h>
#include <dev/timer.h>
#include <machine/isa/spkr.h>
#include <machine/isa/i8254.h>
#include <machine/pio.h>
#include <string.h>

#define DIVIDEND 1193180
#define CTRL_PORT 0x61

static struct cdevsw beep_cdevsw;

/*
 * Write to the pcspkr
 *
 * Bits 15:0 - frequency (hz)
 * Bits 31:16 - duration (msec)
 */
static int
dev_write(dev_t dev, struct sio_txn *sio, int flags)
{
    uint32_t payload = 0;
    uint16_t hz;
    uint16_t duration;
    size_t len = sizeof(payload);

    if (sio->len < len) {
        return -EINVAL;
    }

    memcpy(&payload, sio->buf, len);
    hz = payload & 0xFFFF;
    duration = (payload >> 16) & 0xFFFF;
    pcspkr_tone(hz, duration);
    return sio->len;
}

static int
beep_init(void)
{
    char devname[] = "beep";
    devmajor_t major;
    dev_t dev;

    /* Register the device here */
    major = dev_alloc_major();
    dev = dev_alloc(major);
    dev_register(major, dev, &beep_cdevsw);
    devfs_create_entry(devname, major, dev, 0666);
    return 0;
}

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
    outb(I8254_COMMAND, 0xB6);
    outb(I8254_CHANNEL_2, divisor & 0xFF);
    outb(I8254_CHANNEL_2, (divisor >> 8) & 0xFF);

    /* Oscillate the speaker */
    tmp = inb(CTRL_PORT);
    if (!ISSET(tmp, 3)) {
        tmp |= 3;
        outb(CTRL_PORT, tmp);
    }

    /* Sleep then turn off the speaker */
    tmr.msleep(msec);
    outb(CTRL_PORT, tmp & ~3);
    return 0;
}

static struct cdevsw beep_cdevsw = {
    .read = noread,
    .write = dev_write
};

DRIVER_EXPORT(beep_init);

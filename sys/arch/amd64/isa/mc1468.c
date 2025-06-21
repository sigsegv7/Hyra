/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
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
#include <sys/param.h>
#include <sys/time.h>
#include <sys/driver.h>
#include <sys/device.h>
#include <sys/syslog.h>
#include <fs/devfs.h>
#include <machine/pio.h>
#include <machine/cdefs.h>
#include <string.h>

#define MC1468_REGSEL  0x70
#define MC1468_DATA    0x71

/* Register A flags */
#define MC1468_UPDATING  BIT(7)

/* Register B flags */
#define MC1468_DAYSAVE   BIT(1)
#define MC1468_CLOCK24   BIT(2)

static struct cdevsw mc1468_cdevsw;

/*
 * Read a byte from an MC1468XX register.
 */
static uint8_t
mc1468_read(uint8_t reg)
{
    outb(MC1468_REGSEL, reg);
    return inb(MC1468_DATA);
}

/*
 * Returns true if the MC1468XX is updating
 * its time registers.
 */
static bool
mc1468_updating(void)
{
    uint8_t reg_b;

    reg_b = mc1468_read(0xB);
    return ISSET(reg_b, MC1468_UPDATING) != 0;
}

/*
 * Check if date `a' and date `b' are synced.
 * Used to make sure a bogus date caused by a
 * read right before an MC1468XX register
 * update doesn't occur.
 */
static bool
mc1468_date_synced(struct date *a, struct date *b)
{
    if (a->year != b->year)
        return false;
    if (a->month != b->month)
        return false;
    if (a->day != b->day)
        return false;
    if (a->sec != b->sec)
        return false;
    if (a->min != b->min)
        return false;
    if (a->hour != b->hour)
        return false;

    return true;
}

/*
 * Sometimes the clock chip may encode the
 * date in binary-coded-decimal. This function
 * converts a date in BCD format to plain binary.
 */
static void
mc1468_bcd_conv(struct date *dp)
{
    dp->year = (dp->year & 0x0F) + ((dp->year / 16) * 10);
    dp->month = (dp->month & 0x0F) + ((dp->month / 16) * 10);
    dp->day = (dp->day & 0x0F) + ((dp->day / 16) * 10);
    dp->sec = (dp->sec & 0x0F) + ((dp->sec / 16) * 10);
    dp->min = (dp->min & 0x0F) + ((dp->min / 16) * 10);
    dp->hour = (dp->hour & 0x0F) + (((dp->hour & 0x70) / 16) * 10);
    dp->hour |= dp->hour & 0x80;
}

/*
 * Read the time for the clock without syncing
 * it up.
 *
 * XXX: Please use mc1468_get_date() instead as
 *      this function may return inconsistent
 *      values if not used correctly.
 */
static void
__mc1468_get_time(struct date *dp)
{
    dp->year = mc1468_read(0x09);
    dp->month = mc1468_read(0x08);
    dp->day = mc1468_read(0x07);
    dp->sec = mc1468_read(0x00);
    dp->min = mc1468_read(0x02);
    dp->hour = mc1468_read(0x04);
}

static int
mc1468_get_date(struct date *dp)
{
    struct date date_cur, date_last;
    uint8_t reg_b = mc1468_read(0x0B);

    while (mc1468_updating()) {
        __mc1468_get_time(&date_last);
    }

    /*
     * Get the current date and time.
     *
     * XXX: The date and time returned by __mc1468_get_time()
     *      may at times be out of sync, read it twice to
     *      make sure everything is synced up.
     */
    do {
        while (mc1468_updating()) {
            md_pause();
        }
        __mc1468_get_time(&date_last);
        date_cur.year = date_last.year;
        date_cur.month = date_last.month;
        date_cur.day = date_last.day;
        date_cur.sec = date_last.sec;
        date_cur.min = date_last.min;
        date_cur.hour = date_last.hour;
    } while (!mc1468_date_synced(&date_cur, &date_last));

    /* Is this in BCD? */
    if (!ISSET(reg_b, 0x04)) {
        mc1468_bcd_conv(&date_cur);
    }

    /* 24-hour mode? */
    if (ISSET(reg_b, MC1468_CLOCK24)) {
        date_cur.hour = ((date_cur.hour & 0x7F) + 12) % 24;
    }

    *dp = date_cur;
    return 0;
}

static int
mc1468_dev_read(dev_t dev, struct sio_txn *sio, int flags)
{
    struct date d;
    size_t len = sizeof(d);

    if (sio->len > len) {
        sio->len = len;
    }

    mc1468_get_date(&d);
    memcpy(sio->buf, &d, sio->len);
    return sio->len;
}

static int
mc1468_init(void)
{
    char devname[] = "rtc";
    devmajor_t major;
    dev_t dev;

    major = dev_alloc_major();
    dev = dev_alloc(major);
    dev_register(major, dev, &mc1468_cdevsw);
    devfs_create_entry(devname, major, dev, 0444);
    return 0;
}

static struct cdevsw mc1468_cdevsw = {
    .read = mc1468_dev_read,
    .write = nowrite
};

DRIVER_EXPORT(mc1468_init);

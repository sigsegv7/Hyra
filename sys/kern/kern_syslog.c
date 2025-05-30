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

#include <sys/syslog.h>
#include <sys/cdefs.h>
#include <sys/sio.h>
#include <sys/spinlock.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <dev/cons/cons.h>
#include <dev/timer.h>
#include <fs/devfs.h>
#include <stdarg.h>
#include <string.h>

#if defined(__SERIAL_DEBUG)
#define SERIAL_DEBUG __SERIAL_DEBUG
#else
#define SERIAL_DEBUG 0
#endif

#if defined(__USER_KMSG)
#define USER_KMSG __USER_KMSG
#else
#define USER_KMSG 0
#endif

#define KBUF_SIZE (1 << 16)

/* Sanity check */
__static_assert(KBUF_SIZE <= (1 << 16), "KBUF_SIZE too high!");

/* Global logger lock */
static struct spinlock lock = {0};
static struct spinlock kmsg_lock = {0};
static bool no_cons_log = false;

/* Kernel message buffer */
static char kmsg[KBUF_SIZE];
static size_t kmsg_i = 0;
static struct cdevsw kmsg_cdevw;

static void
kmsg_append(const char *s, size_t len)
{
    spinlock_acquire(&kmsg_lock);
    if ((kmsg_i + len) >= KBUF_SIZE) {
        kmsg_i = 0;
    }

    for (size_t i = 0; i < len; ++i) {
        kmsg[kmsg_i + i] = s[i];
    }
    kmsg_i += len;
    spinlock_release(&kmsg_lock);
}

/*
 * Character device function.
 */
static int
kmsg_read(dev_t dev, struct sio_txn *sio, int flags)
{
    size_t len, offset, j;
    size_t bytes_read = 0;
    char *p = sio->buf;

    spinlock_acquire(&kmsg_lock);
    len = sio->len;
    offset = sio->offset;

    if (len == 0) {
        spinlock_release(&kmsg_lock);
        return -EINVAL;
    }
    if (offset >= kmsg_i) {
        spinlock_release(&kmsg_lock);
        return 0;
    }

    for (size_t i = 0; i < len; ++i) {
        j = offset + i;
        if (j > kmsg_i) {
            break;
        }

        p[i] = kmsg[j];
        ++bytes_read;
    }

    spinlock_release(&kmsg_lock);
    return bytes_read;
}

static void
syslog_write(const char *s, size_t len)
{
    const char *p;
    size_t l;

    if (SERIAL_DEBUG) {
        p = s;
        l = len;
        while (l--) {
            serial_putc(*p);
            ++p;
        }
    }

    kmsg_append(s, len);

    /*
     * If the USER_KMSG option is disabled in kconf,
     * do not log to the console if everything else
     * has already started.
     */
    if (!USER_KMSG && no_cons_log) {
        return;
    }

    cons_putstr(&g_root_scr, s, len);
}

/*
 * XXX: Not serialized
 */
void
vkprintf(const char *fmt, va_list *ap)
{
    char buffer[1024] = {0};

    vsnprintf(buffer, sizeof(buffer), fmt, *ap);
    syslog_write(buffer, strlen(buffer));
}

void
kprintf(const char *fmt, ...)
{
    va_list ap;
    char timestamp[64] = "[  0.000000] ";
    const char *fmt_p = fmt;
    bool use_timestamp = true;
    bool has_counter = true;
    struct timer tmr;

    /*
     * If the first char is OMIT_TIMESTAMP, then we won't
     * print out the timestamp.
     */
    if (*fmt == OMIT_TIMESTAMP[0]) {
        ++fmt_p;
        use_timestamp = false;
    }

    /* See if we have a timer */
    if (req_timer(TIMER_GP, &tmr) != TMRR_SUCCESS) {
        has_counter = false;
    }

    /* If we can use the counter, format the timestamp */
    if (has_counter) {
        if (tmr.get_time_sec != NULL && tmr.get_time_usec != NULL) {
            snprintf(timestamp, sizeof(timestamp), "[  %d.%06d] ",
                tmr.get_time_sec(), tmr.get_time_usec());
        }
    }

    spinlock_acquire(&lock);
    if (use_timestamp) {
        syslog_write(timestamp, strlen(timestamp));
    }

    va_start(ap, fmt);

    vkprintf(fmt_p, &ap);
    va_end(ap);
    spinlock_release(&lock);
}

/*
 * Silence kernel messages in if the system
 * is already operating in a user context.
 *
 * XXX: This is ignored if the kconf USER_KMSG
 *      option is set to "no". A kmsg device file
 *      is also created on the first call.
 */
void
syslog_silence(bool option)
{
    static bool once = false;
    static char devname[] = "kmsg";
    devmajor_t major;
    dev_t dev;

    if (!once) {
        once = true;
        major = dev_alloc_major();
        dev = dev_alloc(major);

        dev_register(major, dev, &kmsg_cdevw);
        devfs_create_entry(devname, major, dev, 0444);

    }

    no_cons_log = option;
}

static struct cdevsw kmsg_cdevw = {
    .read = kmsg_read,
    .write = nowrite
};

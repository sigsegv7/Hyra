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

#include <sys/tty.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/ascii.h>
#include <fs/devfs.h>
#include <string.h>

static dev_t tty_major = 0;
struct tty g_root_tty = {
    .scr = &g_syslog_screen,
    .ring = {
        .enq_index = 0,
        .deq_index = 0,
    }
};

static inline dev_t
tty_alloc_id(void)
{
    static dev_t id = 1;

    return id++;
}

/*
 * Flushes the TTY ring buffer.
 *
 * @tty: TTY to flush.
 *
 * Returns number of bytes flushed.
 */
static ssize_t
__tty_flush(struct tty *tty)
{
    struct tty_ring *ring = &tty->ring;
    const struct termios *termios;
    size_t count = 0;
    char tmp;

    termios = &tty->termios;

    /* Do we have any data left? */
    if (ring->deq_index >= ring->enq_index)
        return -EAGAIN;

    /*
     * Flush the ring to the console if we are in
     * canonical mode. Otherwise, just throw away
     * the bytes.
     */
    if (__TEST(termios->c_lflag, ICANON)) {
        while (ring->deq_index < ring->enq_index) {
            tmp = ring->data[ring->deq_index++];
            vcons_putch(tty->scr, tmp);
            ++count;
        }
    }

    ring->enq_index = 0;
    ring->deq_index = 0;
    return count;
}

/*
 * Device structure routine for reading a
 * TTY device file.
 */
static int
tty_dev_read(struct device *dev, struct sio_txn *sio)
{
    struct tty_ring *ring = &g_root_tty.ring;
    size_t len;

    spinlock_acquire(&g_root_tty.rlock);
    len = (ring->enq_index - ring->deq_index);

    /*
     * Transfer data from the TTY ring with SIO and
     * flush it.
     *
     * TODO: As of now we are just reading the root
     *       TTY, add support for multiple TTYs.
     */
    memcpy(sio->buf, ring->data, len);
    __tty_flush(&g_root_tty);

    spinlock_release(&g_root_tty.rlock);
    return len;
}

/*
 * Serialized wrapper over __tty_flush()
 */
ssize_t
tty_flush(struct tty *tty)
{
    ssize_t ret;

    spinlock_acquire(&tty->rlock);
    ret = __tty_flush(tty);
    spinlock_release(&tty->rlock);
    return ret;
}

/*
 * Write a character to a TTY
 *
 * @tty: TTY to write to.
 * @c: Character to write.
 */
int
tty_putc(struct tty *tty, int c)
{
    struct tty_ring *ring;
    const struct termios *termios;
    bool canon;

    ring = &tty->ring;
    termios = &tty->termios;
    canon = __TEST(termios->c_lflag, ICANON);

    spinlock_acquire(&tty->rlock);
    ring->data[ring->enq_index++] = c;

    /*
     * If we aren't in canonical mode, just write directly to
     * the console.
     *
     * XXX: Won't need to worry about extra bytes being printed
     *      as __tty_flush() won't flush them to the console
     *      when we aren't in canonical mode.
     */
    if (!canon) {
        vcons_putch(tty->scr, c);
    }

    /*
     * If we are in canonical mode and we have a linefeed ('\n')
     * character, we should flush the ring.
     */
    if (canon && c == ASCII_LF) {
        __tty_flush(tty);
    }

    /* Flush if the buffer is full */
    if (ring->enq_index >= TTY_RING_SIZE) {
        __tty_flush(tty);
    }

    spinlock_release(&tty->rlock);
    return 0;
}

/*
 * Write a string to a TTY
 *
 * @tty: TTY to write to.
 * @s: String to write.
 * @count: Number of bytes to write.
 */
int
tty_putstr(struct tty *tty, const char *s, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        tty_putc(tty, *s++);
    }

    return 0;
}

dev_t
tty_attach(struct tty *tty)
{
    int tmp;
    char devname[128];
    struct device *dev = device_alloc();

    if (dev == NULL)
        return -ENOMEM;

    /*
     * Allocate a major for the driver if we don't
     * have one yet.
     */
    if (tty_major == 0)
        tty_major = device_alloc_major();

    /* Now try to create the device */
    tty->id = tty_alloc_id();
    if ((tmp = device_create(dev, tty_major, tty->id)) < 0)
        return tmp;

    dev->read = tty_dev_read;
    dev->blocksize = 1;

    snprintf(devname, sizeof(devname), "tty%d", tty->id);
    return devfs_add_dev(devname, dev);
}

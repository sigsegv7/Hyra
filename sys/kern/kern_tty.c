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

struct tty g_root_tty = {
    .scr = &g_syslog_screen,
    .ring = {
        .enq_index = 0,
        .deq_index = 0,
    }
};

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
    size_t count = 0;
    char tmp;

    /* Do we have any data left? */
    if (ring->deq_index >= ring->enq_index)
        return -EAGAIN;

    /* Flush the ring */
    while (ring->deq_index < ring->enq_index) {
        tmp = ring->data[ring->deq_index++];
        vcons_putch(tty->scr, tmp);
        ++count;
    }

    ring->enq_index = 0;
    ring->deq_index = 0;
    return count;
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

    ring = &tty->ring;
    spinlock_acquire(&tty->rlock);

    if (ring->enq_index >= TTY_RING_SIZE) {
        __tty_flush(tty);
    }

    ring->data[ring->enq_index++] = c;
    spinlock_release(&tty->rlock);
    return 0;
}

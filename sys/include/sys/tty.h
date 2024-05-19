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

#ifndef _SYS_TTY_H_
#define _SYS_TTY_H_

#include <sys/termios.h>
#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/spinlock.h>
#include <sys/device.h>
#include <dev/vcons/vcons.h>
#endif

/* TTY ioctl commands */
#define TCSETS 0x00000000U
#define TCGETS 0x00000001U

#if defined(_KERNEL)
#define TTY_RING_SIZE 32
#define TTY_SOURCE_RAW 0x0001U   /* Raw text */
#define TTY_SOURCE_DEV 0x0002U   /* Input from device (e.g keyboard) */

struct tty_ring {
    char data[TTY_RING_SIZE];   /* Ring data */
    off_t enq_index;            /* Enqueue index */
    off_t deq_index;            /* Dequeue index */
};

struct tty {
    dev_t id;
    struct vcons_screen *scr;   /* Console screen */
    struct tty_ring ring;       /* Input ring */
    struct tty_ring outring;    /* Output ring */
    struct spinlock rlock;      /* Ring lock */
    struct termios termios;     /* Termios structure */
    struct device *dev;         /* Device pointer */
};

extern struct tty g_root_tty;

dev_t tty_attach(struct tty *tty);
int tty_putc(struct tty *tty, int c, int flags);
int tty_putstr(struct tty *tty, const char *s, size_t count);
ssize_t tty_flush(struct tty *tty);

#endif  /* defined(_KERNEL) */
#endif  /* !_SYS_TTY_H_ */

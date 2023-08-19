/*
 * Copyright (c) 2023 Emilia Strange and the VegaOS team.
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

/* $Id$ */

#ifndef _SYS_TTY_H_
#define _SYS_TTY_H_

#include <sys/types.h>
#include <sys/termios.h>
#include <sys/queue.h>
#include <sys/cdefs.h>
#include <sys/spinlock.h>
#include <dev/video/fbdev.h>

/*
 * Max chars an entire line can hold:
 * 2^9 => 512 bytes
 */
#define LINE_RING_SIZE __POW2(9)

/* Default TTY tab width */
#define DEFAULT_TAB_WIDTH   4

/*
 * Describes the size
 * of a TTY window.
 */
struct winsize {
    uint16_t    ws_row;     /* Rows, in chars */
    uint16_t    ws_col;     /* Columns, in chars */
    uint16_t    ws_xpixel;  /* Horizontal size, in pixels */
    uint16_t    ws_ypixel;  /* Vertical size, in pixels */
};

/*
 * TTY ring buffer.
 */
struct tty_ring {
    char buf[LINE_RING_SIZE];       /* Actual buffer */
    size_t len;
};

/*
 * Describes a TTY. Each TTY
 * shall be described by a
 * `struct tty`.
 */
struct tty {
    uint32_t chpos_x;               /* Next char X position */
    uint32_t chpos_y;               /* Next char Y position */
    uint32_t curspos_x;             /* Cursor X position */
    uint32_t curspos_y;             /* Cursor Y position */
    uint32_t fg;                    /* Foreground (hex color) */
    uint32_t bg;                    /* Background (hex color) */
    uint8_t tab_width;              /* Width of a tab (in chars) */
    struct fbdev fbdev;             /* Framebuffer device */
    struct termios termios;         /* Termios state */
    struct winsize winsize;         /* Window size */
    struct tty_ring ring;          /* Ring buffer */
    struct spinlock lock;           /* Protects TTY */
    TAILQ_ENTRY(tty) link;          /* TTY list link */
};

/*
 * Some helper macros to facilitate
 * locking/unlocking a TTY.
 */
#define TTY_LOCK(tty_ptr)   spinlock_acquire(&(tty_ptr)->lock)
#define TTY_UNLOCK(tty_ptr) spinlock_release(&(tty_ptr)->lock)

/*
 * Macros to allow easy access for
 * termios and winsize flags, e.g usage:
 *
 * struct tty *tty = ...
 * tty->t_cflag = ...
 */
#define t_cflag     termios.c_cflag
#define t_iflag     termios.c_iflag
#define t_lflag     termios.c_lflag
#define t_oflag     termios.c_oflag
#define t_ispeed    termios.c_ispeed
#define t_ospeed    termios.c_ospeed
#define t_ws_row    winsize.ws_row
#define t_ws_col    winsize.ws_col
#define t_ws_xpixel winsize.ws_xpixel
#define t_ws_ypixel winsize.ws_ypixel

void tty_push_char(struct tty *tty, int c);
void tty_flush(struct tty *tty);
ssize_t tty_write(struct tty *tty, const char *buf, size_t len);
void tty_set_defaults(struct tty *tty);
void tty_attach(struct tty *tty);
void tty_init(void);

#endif  /* !_SYS_TTY_H_ */

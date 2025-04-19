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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ascii.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <dev/video/fbdev.h>
#include <dev/cons/font.h>
#include <dev/cons/cons.h>
#include <fs/devfs.h>
#include <vm/dynalloc.h>
#include <string.h>

#define HIDE_CURSOR(SCR) \
    cons_draw_cursor((SCR), (SCR)->bg)

#define SHOW_CURSOR(SCR) \
    cons_draw_cursor((SCR), (SCR)->fg)

/* Console background from kconf */
#if defined(__CONSOLE_BG)
#define CONSOLE_BG __CONSOLE_BG
#else
#define CONSOLE_BG 0x000000
#endif /* __CONSOLE_BG */

/* Console foreground from kconf */
#if defined(__CONSOLE_FG)
#define CONSOLE_FG __CONSOLE_FG
#else
#define CONSOLE_FG 0x00AA00
#endif  /* __CONSOLE_FG */

struct cons_screen g_root_scr = {0};
static struct cdevsw cons_cdevsw;

static void cons_draw_cursor(struct cons_screen *scr, uint32_t color);
static int cons_handle_special(struct cons_screen *scr, char c);
static void cons_clear_scr(struct cons_screen *scr, uint32_t bg);

/*
 * Render a character onto the screen.
 *
 * @scr: Screen to render to.
 * @c: Char to render.
 * @x: X position of char.
 * @y: Y position of char.
 */
static void
cons_draw_char(struct cons_screen *scr, struct cons_char ch)
{
    size_t idx;
    uint32_t x, y;
    const uint8_t *glyph;

    if (scr->fb_mem == NULL) {
        return;
    }

    glyph = &CONS_FONT[(int)ch.c*16];
    x = ch.x;
    y = ch.y;

    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            idx = fbdev_get_index(&scr->fbdev, x + (FONT_WIDTH - 1) - cx, y + cy);
            scr->fb_mem[idx] = ISSET(glyph[cy], BIT(cx)) ? ch.fg : ch.bg;
        }
    }
}

/*
 * Internal helper - flush console row
 *
 * @row: Row to flush.
 */
static int
cons_flush_row(struct cons_screen *scr, uint32_t row)
{
    struct cons_buf *bp;
    struct cons_char cc;

    bp = scr->ob[row];
    if (ISSET(bp->flags, CONS_BUF_CLEAN)) {
        return -EIO;
    }

    for (int j = 0; j < bp->len; ++j) {
        if (cons_obuf_pop(bp, &cc) != 0) {
            continue;
        }

        cons_draw_char(scr, cc);
        bp->flags |= CONS_BUF_CLEAN;
    }

    return 0;
}

/*
 * Internal helper - flush console
 *
 */
static int
cons_flush(struct cons_screen *scr)
{
    for (int i = 0; i < scr->nrows; ++i) {
        cons_flush_row(scr, i);
    }
    return 0;
}

/*
 * Handle a special character (e.g  "\t", "\n", etc)
 *
 * @scr: Screen to handle this on.
 * @c: Char to handle.
 *
 * Returns 0 if handled, otherwise -1.
 */
static int
cons_handle_special(struct cons_screen *scr, char c)
{
    if (scr->ch_col >= scr->ncols - 20) {
        scr->ch_col = 0;
        cons_handle_special(scr, '\n');
    }

    switch (c) {
    case ASCII_LF:
        HIDE_CURSOR(scr);

        /* Are we past screen width? */
        if (scr->ch_row >= scr->nrows - 1) {
            cons_clear_scr(scr, scr->bg);
            cons_flush(scr);
            scr->ch_col = 0;
            scr->ch_row = 0;

            /* Update cursor */
            scr->curs_row = 0;
            scr->curs_col = 0;
            SHOW_CURSOR(scr);
            return 0;
        }

        /* Make a newline */
        cons_flush(scr);
        ++scr->ch_row;
        scr->ch_col = 0;
        cons_flush(scr);

        /* Update cursor */
        scr->curs_row += 1;
        scr->curs_col = 0;
        SHOW_CURSOR(scr);
        return 0;
    case ASCII_FF:
        /*
         * Fuck what they say, this is now the
         * "flush" byte ::)
         */
        cons_flush(scr);
        return 0;
    }

    return -1;
}

static void
cons_draw_cursor(struct cons_screen *scr, uint32_t color)
{
    size_t idx;

    /* Past screen width? */
    if (scr->curs_col >= scr->ncols) {
        scr->curs_col = 0;
        scr->curs_row++;
    }

    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            idx = fbdev_get_index(&scr->fbdev, (scr->curs_col * FONT_WIDTH) + cx, (scr->curs_row * FONT_HEIGHT) + cy);
            scr->fb_mem[idx] = color;
        }
    }
}

/*
 * Clear the screen.
 *
 * @scr: Screen to clear.
 * @bg: Color to clear it to.
 */
static void
cons_clear_scr(struct cons_screen *scr, uint32_t bg)
{
    struct fbdev fbdev = scr->fbdev;
    struct cons_buf *bp;

    for (size_t i = 0; i < fbdev.height * fbdev.pitch; ++i) {
        scr->fb_mem[i] = bg;
    }

    bp = scr->ob[scr->nrows - 1];
    bp->flags |= CONS_BUF_CLEAN;
}

/*
 * Character device function.
 */
static int
dev_write(dev_t dev, struct sio_txn *sio, int flags)
{
    char *p;

    p = sio->buf;

    for (size_t i = 0; i < sio->len; ++i) {
        cons_putch(&g_root_scr, p[i]);
    }

    cons_flush(&g_root_scr);
    return sio->len;
}

/*
 * Character device function.
 */
static int
dev_read(dev_t dev, struct sio_txn *sio, int flags)
{
    struct cons_input input;
    uint8_t *p;
    int retval;
    size_t n;

    p = (uint8_t *)sio->buf;
    n = sio->len;

    /* Must be a power of two */
    if ((n & 1) != 0) {
        return -EFAULT;
    }

    retval = cons_ibuf_pop(&g_root_scr, &input);
    if (retval < 0) {
        return -EAGAIN;
    }

    spinlock_acquire(&g_root_scr.lock);
    for (;;) {
        /* Buffer too small */
        if (n == 0) {
            break;
        }

        *p++ = input.chr;
        *p++ = input.scancode;
        n -= 2;

        /* Try to get the next byte */
        retval = cons_ibuf_pop(&g_root_scr, &input);
        if (retval < 0) {
            break;
        }
    }
    spinlock_release(&g_root_scr.lock);
    return sio->len;
}

static int
cons_init_bufs(struct cons_screen *scr)
{
    struct cons_buf *bp;
    size_t ob_len;

    scr->ib = cons_new_buf(CONS_BUF_INPUT, scr->ncols);
    if (g_root_scr.ib == NULL) {
        panic("out of memory\n");
    }

    ob_len = sizeof(*scr->ob) * scr->nrows;
    scr->ob = dynalloc(ob_len);

    /* Allocate all output buffers per line */
    for (size_t i = 0; i < scr->nrows; ++i) {
        bp = cons_new_buf(CONS_BUF_OUTPUT, scr->ncols);
        if (bp == NULL) {
            panic("out of memory\n");
        }
        bp->flags |= CONS_BUF_CLEAN;
        scr->ob[i] = bp;
    }

    return 0;
}

/*
 * Put a character on the screen.
 *
 * @scr: Screen.
 * @c: Character to draw.
 */
int
cons_putch(struct cons_screen *scr, char c)
{
    struct cons_buf *bp;
    struct cons_char cc;
    size_t max_width;

    spinlock_acquire(&scr->lock);

    /* Handle specials */
    if (cons_handle_special(scr, c) == 0) {
        goto done;
    }

    HIDE_CURSOR(scr);

    /* Create a new character */
    cc.c = c;
    cc.fg = scr->fg;
    cc.bg = scr->bg;
    cc.x = scr->ch_col * FONT_WIDTH;
    cc.y = scr->ch_row * FONT_HEIGHT;

    /* Push our new character */
    bp = scr->ob[scr->ch_row];
    bp->flags &= ~CONS_BUF_CLEAN;
    cons_obuf_push(bp, cc);
    ++scr->ch_col;

    /* Check screen bounds */
    max_width = scr->ncols * FONT_WIDTH;
    if (cc.x >= max_width - 1) {
        scr->ch_col = 0;
        ++scr->ch_row;
    }

    ++scr->curs_col;
    if (scr->curs_col > scr->ncols - 1) {
        scr->curs_col = 0;
        if (scr->curs_row < scr->nrows)
            ++scr->curs_row;
    }
    SHOW_CURSOR(scr);
done:
    spinlock_release(&scr->lock);
    return 0;
}

void
cons_init(void)
{
    struct fbdev fbdev = fbdev_get();

    g_root_scr.ch_col = 0;
    g_root_scr.ch_row = 0;
    g_root_scr.fg = CONSOLE_FG;
    g_root_scr.bg = CONSOLE_BG;
    g_root_scr.fb_mem = fbdev.mem;
    g_root_scr.nrows = fbdev.height / FONT_HEIGHT;
    g_root_scr.ncols = fbdev.width / FONT_WIDTH;
    g_root_scr.fbdev = fbdev;
    memset(&g_root_scr.lock, 0, sizeof(g_root_scr.lock));
    cons_init_bufs(&g_root_scr);
    SHOW_CURSOR(&g_root_scr);
}

/*
 * Expose the console to /dev/console
 */
void
cons_expose(void)
{
    static int once = 0;
    char devname[] = "console";
    devmajor_t major;
    dev_t dev;

    /* Only run once */
    if (once) {
        return;
    }

    /* Register the device here */
    major = dev_alloc_major();
    dev = dev_alloc(major);
    dev_register(major, dev, &cons_cdevsw);
    devfs_create_entry(devname, major, dev, 0444);
    once ^= 1;
}

static struct cdevsw cons_cdevsw = {
    .read = dev_read,
    .write = dev_write
};

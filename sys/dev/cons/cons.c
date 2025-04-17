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
#include <dev/video/fbdev.h>
#include <dev/cons/font.h>
#include <dev/cons/cons.h>
#include <fs/devfs.h>
#include <vm/dynalloc.h>
#include <string.h>

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

/*
 * Create a chracter descriptor for drawing
 * characters.
 *
 * @c: Character.
 * @fg: Foreground.
 * @bg: Background.
 */
static inline struct cons_char
cons_make_char(char c, uint32_t fg, uint32_t bg)
{
    struct cons_char ch;

    ch.fg = fg;
    ch.bg = bg;
    ch.c = c;
    return ch;
}

/*
 * Render a character onto the screen.
 *
 * @scr: Screen to render to.
 * @c: Char to render.
 * @x: X position of char.
 * @y: Y position of char.
 */
static void
cons_draw_char(struct cons_screen *scr, struct cons_char ch,
                 uint32_t x, uint32_t y)
{
    size_t idx;
    const uint8_t *glyph;

    if (scr->fb_mem == NULL) {
        return;
    }

    glyph = &CONS_FONT[(int)ch.c*16];
    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            idx = fbdev_get_index(&scr->fbdev, x + (FONT_WIDTH - 1) - cx, y + cy);
            scr->fb_mem[idx] = ISSET(glyph[cy], BIT(cx)) ? ch.fg : ch.bg;
        }
    }
}

static void
cons_draw_cursor(struct cons_screen *scr, uint32_t color)
{
    size_t idx;

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

    for (size_t i = 0; i < fbdev.height * fbdev.pitch; ++i) {
        scr->fb_mem[i] = bg;
    }
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
    switch (c) {
    case ASCII_LF:
        /* Make a newline */
        scr->ch_col = 0;
        ++scr->ch_row;

        cons_draw_cursor(scr, scr->bg);
        scr->curs_col = 0;
        scr->curs_row++;
        cons_draw_cursor(scr, scr->last_chr.fg);
        return 0;
    }

    return -1;
}

/*
 * Character device function.
 */
static int
dev_write(dev_t dev, struct sio_txn *sio, int flags)
{
    char *p;

    p = sio->buf;
    spinlock_acquire(&g_root_scr.lock);

    for (size_t i = 0; i < sio->len; ++i) {
        cons_putch(&g_root_scr, p[i]);
    }

    spinlock_release(&g_root_scr.lock);
    return sio->len;
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
    struct cons_char cons_chr;

    if (scr->ch_col > scr->ncols) {
        /* Make a newline as we past the max col */
        scr->ch_col = 0;
        ++scr->ch_row;
    }

    if (scr->curs_row >= scr->nrows) {
        /* Went over the screen size */
        /* TODO: Scroll instead of just clearing the screen */
        scr->ch_col = 0;
        scr->ch_row = 0;
        cons_clear_scr(scr, scr->bg);

        scr->curs_col = 0;
        scr->curs_row = 0;
        cons_draw_cursor(scr, scr->last_chr.fg);
    }

    /*
     * If this is a special char that we can handle
     * then handle it and return.
     */
    if (cons_handle_special(scr, c) == 0) {
        return 0;
    }

    cons_chr = cons_make_char(c, scr->fg, scr->bg);
    scr->last_chr = cons_chr;

    /* Draw cursor and character */
    scr->curs_col++;
    cons_draw_cursor(scr, scr->last_chr.fg);
    cons_draw_char(scr, cons_chr, scr->ch_col * FONT_WIDTH,
        scr->ch_row * FONT_HEIGHT);

    ++scr->ch_col;
    return 0;
}

void
cons_init(void)
{
    struct fbdev fbdev = fbdev_get();

    g_root_scr.fg = CONSOLE_FG;
    g_root_scr.bg = CONSOLE_BG;
    g_root_scr.fb_mem = fbdev.mem;
    g_root_scr.nrows = fbdev.height / FONT_HEIGHT;
    g_root_scr.ncols = fbdev.width / FONT_WIDTH;
    g_root_scr.fbdev = fbdev;
    memset(&g_root_scr.lock, 0, sizeof(g_root_scr.lock));
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
    .read = noread,
    .write = dev_write
};

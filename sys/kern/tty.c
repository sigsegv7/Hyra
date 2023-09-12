/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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

#include <sys/tty.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/ascii.h>
#include <string.h>
#include <tty_font.h>

/*
 * Default cursor color.
 *
 * TODO: The cursor color should
 *       be the invert of the color
 *       of whatever it is on top of.
 */
#define DEFAULT_CURSOR_BG 0x808080

#define CURSOR_WIDTH            FONT_WIDTH
#define CURSOR_HEIGHT           FONT_HEIGHT

__KERNEL_META("$Vega$: tty.c, Ian Marco Moffett, "
              "Core TTY implementation");

/* List of attached TTYs */
static TAILQ_HEAD(, tty) tty_list;

/*
 * Renders a char onto the
 * TTY specified by `tty`.
 */
static void
tty_draw_char(struct tty *tty, char c, uint32_t fg, uint32_t bg)
{
    uint32_t *fb_ptr;
    uint32_t x, y;
    size_t idx;
    const uint8_t *glyph;

    /* Get a pointer to framebuffer memory */
    fb_ptr = tty->fbdev.mem;

    /* Get the specific glyph of `c` */
    glyph = &DEFAULT_FONT_DATA[(int)c*16];

    x = tty->chpos_x;
    y = tty->chpos_y;

    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            idx = fbdev_get_index(&tty->fbdev, x+FONT_WIDTH-cx, y+cy);
            fb_ptr[idx] = __TEST(glyph[cy], __BIT(cx)) ? fg : bg;
        }
    }
}

/*
 * Draws a cursor onto
 * the screen.
 *
 * Call with TTY locked.
 */
static void
tty_draw_cursor(struct tty *tty, bool hide)
{
    uint32_t x, y, idx;
    uint32_t *fb_ptr;
    uint32_t color;

    fb_ptr = tty->fbdev.mem;
    color = hide ? tty->bg : DEFAULT_CURSOR_BG;

    for (size_t cy = 0; cy < CURSOR_HEIGHT; ++cy) {
        for (size_t cx = 0; cx < CURSOR_WIDTH; ++cx) {
            x = tty->curspos_x + cx;
            y = tty->curspos_y + cy;
            idx = fbdev_get_index(&tty->fbdev, x, y);
            fb_ptr[idx] = color;
        }
    }
}

static void
tty_scroll_single(struct tty *tty)
{
    uint32_t *fb_ptr;
    size_t line_size, dest_idx, src_idx;

    fb_ptr = tty->fbdev.mem;
    line_size = tty->fbdev.pitch/4;

    /* Copy each line up */
    for (size_t y = FONT_HEIGHT; y < tty->t_ws_ypixel; y += FONT_HEIGHT) {
        dest_idx = fbdev_get_index(&tty->fbdev, 0, y - FONT_HEIGHT);
        src_idx = fbdev_get_index(&tty->fbdev, 0, y);
        memcpy32(&fb_ptr[dest_idx], &fb_ptr[src_idx], FONT_HEIGHT*line_size);
    }

    /*
     * Ensure we start at X position 0
     * after we scrolled down.
     */
    tty->chpos_x = 0;
    tty->curspos_x = 0;
}

/*
 * Handles a newline.
 *
 * Call with TTY locked.
 */
static inline void
tty_newline(struct tty *tty)
{
    uint32_t ypos;
    const uint32_t MAX_YPOS = tty->t_ws_ypixel - (CURSOR_HEIGHT*2);

    /* Hide the cursor */
    tty_draw_cursor(tty, true);

    /* Reset X positions */
    tty->chpos_x = 0;
    tty->curspos_x = 0;

    /*
     * Get the value closest to end
     * of the screen.
     */
    ypos = __MAX(tty->chpos_y, tty->curspos_y);

    /*
     * Check if we need to scroll
     * instead of incrementing
     * Y positions.
     */
    if (ypos < MAX_YPOS) {
        tty->chpos_y += FONT_HEIGHT;
        tty->curspos_y += FONT_HEIGHT;
    } else {
        tty_scroll_single(tty);
    }

    /* Redraw the cursor */
    tty_draw_cursor(tty, false);
}

/*
 * Appends a character to the TTY specified
 * by `tty` as well as incrementing tty->chpos_x
 * and making newlines as needed.
 *
 * Call with TTY locked.
 */
static inline void
tty_append_char(struct tty *tty, int c)
{
    const uint32_t MAX_XPOS = tty->t_ws_xpixel - FONT_WIDTH;

    /* Hide the cursor */
    tty_draw_cursor(tty, true);

    tty_draw_char(tty, c, tty->fg, tty->bg);
    tty->chpos_x += FONT_WIDTH;
    tty->curspos_x += FONT_WIDTH;

    if (tty->chpos_x >= MAX_XPOS) {
        tty_newline(tty);
    }

    /* Redraw the cursor */
    tty_draw_cursor(tty, false);
}

/*
 * Writes out a tab as `tty->tab_width`
 * spaces.
 *
 * Call with TTY locked.
 */
static void
tty_expand_tab(struct tty *tty)
{
    for (size_t i = 0; i < tty->tab_width; ++i) {
        tty_append_char(tty, ' ');
    }
}

/*
 * Writes a char to the TTY.
 *
 * Returns 0 on success; non-zero value
 * is a character to be resent.
 *
 * Call with TTY locked.
 */
static int
tty_putch(struct tty *tty, int c)
{
    if (!__TEST(tty->t_oflag, OPOST)) {
        /*
         * Just write out the char with
         * no processing.
         */
        tty_append_char(tty, c);
        return 0;
    }

    switch (c) {
    case ASCII_HT:
        /* Tab */
        tty_expand_tab(tty);
        break;
    case ASCII_LF:
        /* Line feed ('\n') */
        tty_newline(tty);
        break;
    default:
        tty_append_char(tty, c);
        break;
    }

    return 0;
}

/*
 * Flushes a TTY specified
 * by `tty`.
 *
 * Call with TTY locked.
 */
void
tty_flush(struct tty *tty)
{
    struct tty_ring *ring;

    ring = &tty->ring;

    /*
     * Write each byte from the buffer
     * to the screen with output processing
     * if possible.
     *
     * XXX: Perhaps there could be a faster
     *      way of doing this instead of
     *      byte by byte?
     */
    for (size_t i = 0; i < ring->len; ++i) {
        tty_putch(tty, ring->buf[i]);
    }

    ring->len = 0;
}

/*
 * Writes to a TTY specified by `tty`.
 *
 * @buf: Buffer to write.
 * @len: Length of buffer.
 *
 * Returns number of bytes written, and
 * EXIT_FAILURE on error.
 */
ssize_t
tty_write(struct tty *tty, const char *buf, size_t len)
{
    if (len == 0) {
        /* Bad value, don't even try */
        return EXIT_FAILURE;
    }

    TTY_LOCK(tty);
    for (size_t i = 0; i < len; ++i) {
        tty_push_char(tty, buf[i]);
        /*
         * If we have a newline and we are
         * buffering bytes, flush the ring.
         */
        if (buf[i] == '\n' && __TEST(tty->t_oflag, ORBUF)) {
            tty_flush(tty);
        }
    }

    /*
     * If we aren't buffering bytes, don't
     * keep the bytes within the ring and
     * flush it right away per `tty_write()`
     * call.
     */
    if (!__TEST(tty->t_oflag, ORBUF)) {
        tty_flush(tty);
    }

    TTY_UNLOCK(tty);
    return len;
}

/*
 * Sets TTY fields to their defaults.
 */
void
tty_set_defaults(struct tty *tty)
{
    /* Ensure everything is initially zero */
    memset(tty, 0, sizeof(*tty));

    /*
     * Now, initialize everything to their defaults.
     *
     * Some notes about the default framebuffer device:
     * ------------------------------------------------
     *  The default framebuffer device should be the
     *  front buffer. Later on during boot, all attached
     *  TTYs shall have their fbdev swapped out with a
     *  backbuffer to improve performace as reading directly
     *  from video memory is going to be slow.
     *
     *  XXX: At some point we should be allocating a backbuffer
     *       instead when it's time for *all* TTYs to have them.
     *
     *       A good idea would be to only allocate a backbuffer
     *       *if* we switched to some TTY and deallocate
     *       that backbuffer when switching away from that TTY.
     *
     *       The first thing that comes to mind when thinking
     *       about this idea is loosing our text when we switch
     *       back out. To rectify this, we could buffer chars
     *       which would take less memory than keeping the whole
     *       backbuffer (holds pixels i.e uint32_t).
     *
     *       This can perhaps be done by some internal flag which
     *       indicates that kmalloc() is usable and chars can
     *       be buffered. Once we switch back, just allocate
     *       a new backbuffer and copy the chars back.
     */
    tty->fbdev = fbdev_get_front();
    tty->t_oflag = (OPOST | ORBUF);
    tty->tab_width = DEFAULT_TAB_WIDTH;
    tty->fg = 0x808080;
    tty->bg = 0x000000;
    tty->t_ws_xpixel = tty->fbdev.width;
    tty->t_ws_ypixel = tty->fbdev.height;
    tty->t_ws_row = tty->fbdev.height / FONT_HEIGHT;
    tty->t_ws_col = tty->fbdev.width / FONT_WIDTH;

}

void
tty_attach(struct tty *tty)
{
    TAILQ_INSERT_TAIL(&tty_list, tty, link);
    tty_draw_cursor(tty, false);
}

void
tty_init(void)
{
    TAILQ_INIT(&tty_list);  /* Ensure the TTY list is usable */
}

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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ascii.h>
#include <dev/video/fbdev.h>
#include <dev/cons/font.h>
#include <dev/cons/cons.h>
#include <vm/dynalloc.h>
#include <string.h>

struct cons_screen g_root_scr = {0};

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
cons_render_char(struct cons_screen *scr, struct cons_char ch,
                 uint32_t x, uint32_t y)
{
    char c = ch.c;
    size_t idx;
    const uint8_t *glyph;

    if (scr->fb_mem == NULL) {
        return;
    }

    glyph = &CONS_FONT[(int)c*16];
    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            idx = fbdev_get_index(&scr->fbdev, x+FONT_WIDTH-cx, y+cy);
            scr->fb_mem[idx] = ISSET(glyph[cy], BIT(cx)) ? ch.fg : ch.bg;
        }
    }
}

static void
cons_draw_cursor(struct cons_screen *scr, uint32_t color)
{
    struct cons_char cursor_chr;

    cursor_chr.fg = scr->fg;
    cursor_chr.bg = color;
    cursor_chr.c = ' ';
    cons_render_char(scr, cursor_chr, scr->curs_col * FONT_WIDTH,
        scr->curs_row * FONT_HEIGHT);
}

static void
cons_move_cursor(struct cons_screen *scr, uint32_t col, uint32_t row)
{
    cons_draw_cursor(scr, scr->bg);
    scr->curs_col = col;
    scr->curs_row = row;
    cons_draw_cursor(scr, scr->last_chr.fg);
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
        cons_move_cursor(scr, 0, scr->curs_row + 1);
        return 0;
    }

    return -1;
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
        scr->ch_col = 0;
        scr->ch_row = 0;
        cons_clear_scr(scr, scr->bg);
        cons_move_cursor(scr, 0, 0);
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
    cons_move_cursor(scr, scr->curs_col + 1, scr->curs_row);
    cons_render_char(scr, cons_chr, scr->ch_col * FONT_WIDTH,
        scr->ch_row * FONT_HEIGHT);

    ++scr->ch_col;
    return 0;
}

void
cons_init(void)
{
    struct fbdev fbdev = fbdev_get();

    g_root_scr.fg = 0x00AA00;
    g_root_scr.bg = 0x000000;
    g_root_scr.fb_mem = fbdev.mem;
    g_root_scr.nrows = fbdev.height / FONT_HEIGHT;
    g_root_scr.ncols = fbdev.width / FONT_WIDTH;
    g_root_scr.fbdev = fbdev;
}

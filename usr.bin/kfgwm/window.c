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

#include <sys/errno.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <kfg/window.h>
#include <kfg/font.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

__always_inline static inline size_t
pixel_index(struct kfg_window *wp, kfgpos_t x, kfgpos_t y)
{
    return x + y * (wp->fb_pitch / 4);
}

static int
kfg_win_putc(struct kfg_window *wp, uint32_t x, uint32_t y, char ch)
{
    size_t idx;
    const uint8_t *glyph;
    uint32_t fg, bg;

    glyph = &g_KFG_FONT[(int)ch*16];
    fg = KFG_WHITE;
    bg = wp->bg;

    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        idx = pixel_index(wp, x + (FONT_WIDTH - 1), y + cy);
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            wp->framebuf[idx--] = ISSET(glyph[cy], BIT(cx)) ? fg : bg;
        }
    }
}

static void
draw_win(struct kfg_window *parent, struct kfg_window *wp)
{
    kfgpixel_t *framep;
    kfgpos_t x_i, y_i;    /* Start */
    kfgpos_t x_end, y_end;    /* End */
    kfgpixel_t brush = wp->bg;
    kfgpos_t rx, ry;  /* Starts at 0 */
    kfgpos_t rx_end, ry_end;  /* Starts at 0 */
    size_t i;

    framep = parent->framebuf;
    x_i = wp->x;
    y_i = wp->y;
    x_end = x_i + wp->width;
    y_end = y_i + wp->height;

    if (x_end > parent->width)
        x_end = parent->width;
    if (y_end > parent->height)
        y_end = parent->height;

    for (kfgpos_t x = x_i; x < x_end; ++x) {
        for (kfgpos_t y = y_i; y < y_i+KFG_TITLE_HEIGHT; ++y) {
            rx = (x - x_i);
            ry = (y - y_i);

            if (rx <= KFG_BORDER_WIDTH && (rx % 2) == 0)
                brush = KFG_WHITE;
            else
                brush = KFG_AQUA;

            i = pixel_index(parent, x, y);
            framep[i] = brush;
        }
    }

    y_i = wp->y + KFG_TITLE_HEIGHT;
    for (kfgpos_t x = x_i; x < x_end; ++x) {
        for (kfgpos_t y = y_i; y < y_end; ++y) {
            rx = (x - x_i);
            ry = (y - y_i);

            if (rx <= KFG_BORDER_WIDTH)
                brush = wp->border_bg;
            else if (ry <= KFG_BORDER_HEIGHT)
                brush = wp->border_bg;
            else if (rx >= (wp->width - KFG_BORDER_WIDTH))
                brush = wp->border_bg;
            else if (ry >= (wp->height - KFG_BORDER_HEIGHT))
                brush = wp->border_bg;
            else
                brush = wp->bg;

            i = pixel_index(parent, x, y);
            framep[i] = brush;
        }
    }
}

/*
 * Draw a window on the screen
 *
 * @parent: Parent window
 * @wp: New window to draw
 *
 * TODO: Double buffering and multiple windows.
 */
int
kfg_win_draw(struct kfg_window *parent, struct kfg_window *wp)
{
    kfgpos_t start_x, start_y;
    kfgpos_t end_x, end_y;
    kfgpos_t max_x, max_y;
    kfgdim_t width, height;

    if (parent == NULL) {
        return -EINVAL;
    }
    if (parent->framebuf == NULL) {
        return -EINVAL;
    }

    max_x = wp->x + parent->width;
    max_y = wp->y + parent->height;

    /* Don't overflow the framebuffer! */
    if ((wp->x + wp->width) > max_x) {
        wp->x = max_x;
    }
    if ((wp->y + wp->height) > max_y) {
        wp->y = max_y;
    }

    draw_win(parent, wp);
    return 0;
}

/*
 * Create a new default window
 *
 * @x: X position for this window
 * @y: Y position for this window
 * @w: Window width
 * @h: Window height
 */
struct kfg_window *
kfg_win_new(struct kfg_window *parent, kfgpos_t x, kfgpos_t y)
{
    struct kfg_window *wp;

    if ((wp = malloc(sizeof(*wp))) == NULL) {
        return NULL;
    }

    wp->x = x;
    wp->y = y;
    wp->width = 250;
    wp->height = 150;
    wp->fb_pitch = parent->fb_pitch;
    wp->framebuf = parent->framebuf;
    wp->bg = KFG_DARK;
    wp->border_bg = KFG_RED;
    return wp;
}

int
kfg_win_putstr(struct kfg_window *wp, struct kfg_text *tp)
{
    size_t slen;
    const char *p;
    kfgpos_t x, y;

    if (tp == NULL)
        return -EINVAL;
    if (tp->text == NULL)
        return -EINVAL;

    slen = strlen(tp->text);
    x = (wp->x + tp->x) + (KFG_BORDER_WIDTH + 1);
    y = (KFG_TITLE_HEIGHT + wp->y) + tp->y;
    p = tp->text;

    while (slen--) {
        if (y >= wp->height) {
            break;
        }

        kfg_win_putc(wp, x, y, *(p++));
        x += FONT_WIDTH;
        if (x >= wp->width) {
            y += FONT_HEIGHT;
            x = wp->x + (KFG_BORDER_WIDTH + 1);
        }
    }

    return 0;
}

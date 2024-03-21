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

#include <dev/vcons/vcons.h>
#include <dev/vcons/vcons_io.h>
#include <dev/video/fbdev.h>
#include <sys/syslog.h>
#include <sys/cdefs.h>
#include <string.h>

__MODULE_NAME("kern_vcons");
__KERNEL_META("$Hyra$: kern_vcons.c, Ian Marco Moffett, "
              "Hyra video console code");

/* Get x or y values in pixels */
#define PIX_CPY_X(scrptr) ((scrptr)->cpy_x * FONT_WIDTH)
#define PIX_CPY_Y(scrptr) ((scrptr)->cpy_y * FONT_HEIGHT)

#define PIX_BOUNDS_MAX_X(scrptr) ((scrptr)->fbdev.width - FONT_WIDTH)
#define PIX_BOUNDS_MAX_Y(scrptr) ((scrptr)->fbdev.height - FONT_HEIGHT)

static struct vcons_screen *screen = NULL;

/*
 * Draw the console cursor.
 *
 * @color: Cursor color
 */
static void
vcons_draw_cursor(struct vcons_screen *scr, uint32_t color)
{
    struct vcons_cursor *cursor = &scr->cursor;
    struct fbdev fbdev = scr->fbdev;

    uint32_t *fbdev_mem = fbdev.mem;
    uint32_t fbdev_idx = 0;
    uint32_t cx, cy;

    for (size_t y = VCONS_CURSOR_HEIGHT; y > 0; --y) {
        for (size_t x = VCONS_CURSOR_WIDTH; x > 0; --x) {
            cx = cursor->old_xpos + x;
            cy = cursor->old_ypos + y;

            fbdev_idx = fbdev_get_index(&fbdev, cx, cy);
            fbdev_mem[fbdev_idx] = color;
        }
    }
}

/*
 * Clear everything out of the console.
 */
static void
vcons_clear_scr(struct vcons_screen *scr)
{
    struct fbdev fbdev = scr->fbdev;

    scr->cpy_x = 0, scr->cpy_y = 0;

    memset(scr->fbdev_mem, scr->bg, (fbdev.pitch * fbdev.height));
    vcons_update_cursor(scr);
}

/*
 * Renders a char onto the screen specified by `scr`.
 *
 * @x,y: In chars
 */
static void
vcons_draw_char(struct vcons_screen *scr, char c, uint32_t x, uint32_t y)
{
    uint32_t *fb_ptr;
    size_t idx;
    const uint8_t *glyph;

    /* Get a pointer to framebuffer memory */
    fb_ptr = scr->fbdev_mem;

    /* Get the specific glyph of `c` */
    glyph = &DEFAULT_FONT_DATA[(int)c*16];

    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            idx = fbdev_get_index(&scr->fbdev, x+FONT_WIDTH-cx, y+cy);
            fb_ptr[idx] = __TEST(glyph[cy], __BIT(cx)) ? scr->fg : scr->bg;
        }
    }
}

/*
 * Update the cursor position.
 *
 * XXX: This function also accounts for the old cursor
 *      and clears it before drawing the new cursor.
 */
void
vcons_update_cursor(struct vcons_screen *scr)
{
    struct vcons_cursor *cursor = &scr->cursor;

    cursor->is_drawing = true;

    if (cursor->is_drawn) {
        /* Clear old cursor */
        vcons_draw_cursor(scr, scr->bg);
    }

    cursor->old_xpos = cursor->xpos;
    cursor->old_ypos = cursor->ypos;
    vcons_draw_cursor(scr, scr->fg);

    cursor->is_drawn = true;
    cursor->is_drawing = false;
}

/*
 * Write out a character on the console.
 *
 * @c: Character to write.
 */
int
vcons_putch(struct vcons_screen *scr, char c)
{
    uint32_t x = PIX_CPY_X(scr);
    uint32_t y = PIX_CPY_Y(scr);
    struct vcons_cursor *cursor = &scr->cursor;
    bool cursor_newline = false;

    while (cursor->is_drawing);

    if (scr == NULL) {
        return 1;
    }

    if (vcons_process_output(scr, c) >= 0) {
        /* No need to do anything */
        return 0;
    }

    /* Check cursor bounds */
    if (cursor->xpos >= PIX_BOUNDS_MAX_X(scr)) {
        cursor->xpos = FONT_WIDTH;
        cursor->ypos += FONT_HEIGHT;
        cursor_newline = true;
    }
    if (cursor->ypos >= PIX_BOUNDS_MAX_Y(scr)) {
        cursor->xpos = FONT_WIDTH;
        cursor->ypos = 0;
    }

    /* Check text bounds */
    if (x >= PIX_BOUNDS_MAX_X(scr)) {
        /* Wrap to the next row */
        ++scr->cpy_y, scr->cpy_x = 0;
        x = PIX_CPY_X(scr), y = PIX_CPY_Y(scr);
    }
    if (y >= PIX_BOUNDS_MAX_Y(scr)) {
        scr->cpy_y = 0;
        scr->cpy_x = 0;
        vcons_clear_scr(scr);
        x = PIX_CPY_X(scr), y = PIX_CPY_Y(scr);
    }

    if (!cursor_newline) {
        cursor->xpos += FONT_WIDTH;
    }

    vcons_update_cursor(scr);
    vcons_draw_char(scr, c, x, y);
    ++scr->cpy_x;
    return 0;
}

/*
 * Write out a string on the console.
 *
 * @s: String to write.
 */
int
vcons_putstr(struct vcons_screen *scr, const char *s, size_t len)
{
    int status;

    for (size_t i = 0; i < len; ++i) {
        if ((status = vcons_putch(scr, s[i])) != 0) {
            return status;
        }
    }

    return 0;
}

void
vcons_attach(struct vcons_screen *scr)
{
    scr->fbdev = fbdev_get_front();
    scr->fbdev_mem = scr->fbdev.mem;

    scr->nrows = scr->fbdev.height;
    scr->ncols = scr->fbdev.width;

    screen = scr;
    vcons_clear_scr(scr);
}

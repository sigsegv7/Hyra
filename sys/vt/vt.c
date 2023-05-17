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

#include <vt/vt.h>
#include <vt/font.h>
#include <dev/video/fb.h>
#include <errno.h>
#include <string.h>

#define DEFAULT_CURSOR_BG   0x5A5A5A
#define DEFAULT_CURSOR_TYPE CURSOR_BLOCK

#define CURSOR_BLOCK_WIDTH 10
#define CURSOR_BLOCK_HEIGHT FONT_HEIGHT

#define NEWLINE_OFF_Y (FONT_HEIGHT*2)

/*
 * Fetches the col from a
 * char and x, y position set.
 */

static inline uint16_t
vt_get_char_col(char c, uint32_t cx, uint32_t cy)
{
    return (DEFAULT_FONT_DATA[(uint64_t)c * FONT_WIDTH + cx] >> cy) & 1;
}

/*
 * Draws the cursor visual for a specific
 * virtual terminal.
 *
 * @vt: Virtual terminal.
 * @color: Cursor color.
 */

static void
vt_draw_cursor(struct vt_descriptor *vt, uint32_t color)
{
    size_t cursor_width = 0;
    size_t cursor_height = 0;
    size_t cursor_x = 0;
    size_t cursor_y = vt->state.cursor_y;
 
    if (vt->state.cursor_x != 0) {
        cursor_x = vt->state.cursor_x + FONT_WIDTH;
    }

    switch (vt->attr.cursor_type) {
        case CURSOR_BLOCK:
            cursor_width = CURSOR_BLOCK_WIDTH;
            cursor_height = CURSOR_BLOCK_HEIGHT;
            break;
    }

    for (size_t cx = cursor_x; cx < cursor_x + cursor_width; ++cx) {
        for (size_t cy = cursor_y; cy < cursor_y + cursor_height; ++cy) {
            fb_put_pixel(vt->fb_base, cx, cy, color);
        }
    }
}

/*
 * Shows the cursor visual for a specific
 * virtual terminal.
 *
 * @vt: Virtual terminal.
 */

static void
vt_show_cursor(struct vt_descriptor *vt)
{
    if (vt->attr.cursor_type == CURSOR_NONE) {
        return;
    }

    vt_draw_cursor(vt, vt->attr.cursor_bg);
}

/*
 * Hides the cursor visual for a specific
 * virtual terminal.
 *
 * @vt: Virtual terminal.
 */

static void
vt_hide_cursor(struct vt_descriptor *vt)
{
    vt_draw_cursor(vt, vt->attr.text_bg);
}

/*
 * Scrolls down one line.
 *
 * @vt: Virtual terminal to scroll.
 *
 * The logic is rather simple, we just
 * start at the first line, copy each
 * line up and clear the first line.
 */

static void
vt_scroll_single(struct vt_descriptor *vt)
{
    size_t line_size = fb_get_pitch()/4;
    
    /* Copy each line up */
    for (size_t y = FONT_HEIGHT; y < fb_get_height(); y += FONT_HEIGHT) {
        memcpy32(&vt->fb_base[fb_get_index(0, y - FONT_HEIGHT)],
                 &vt->fb_base[fb_get_index(0, y)],
                 FONT_HEIGHT*line_size);
    }
    
    /* Clear the first line */
    memset32(vt->fb_base, vt->attr.bg, line_size);
}

/*
 * Make a newline for the
 * specified virtual terminal.
 *
 * @vt: Virtual terminal.
 */

static void
vt_newline(struct vt_descriptor *vt)
{
    vt_hide_cursor(vt);
    vt->state.cursor_x = 0;
    
    /* minus FONT_HEIGHT*4 to give some room for the cursor */
    if (vt->state.cursor_y > fb_get_height() - (FONT_HEIGHT*4)) {
        vt_scroll_single(vt);
    } else {
        vt->state.cursor_y += FONT_HEIGHT;
    }

    vt_show_cursor(vt);
}

/*
 * Renders a character onto a
 * specific virtual terminal.
 *
 * @x, y: Position on the screen.
 * @c: Character to render.
 * @fg: Foreground.
 * @bg: Background.
 */

static void
vt_draw_char(struct vt_descriptor *vt, char c, uint32_t fg, uint32_t bg)
{
    c -= 32;

    uint32_t x = vt->state.cursor_x;
    uint32_t y = vt->state.cursor_y;

    for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
        for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
            uint16_t col = vt_get_char_col(c, cx, cy);
            fb_put_pixel(vt->fb_base, x+cx, y+cy, col ? fg : bg);
        }
    }
}

/*
 * Appends a character to the
 * specified virtual terminal.
 *
 * NOTE: Assumes caller has acquired
 *       vt->lock.
 */

static void
vt_putch(struct vt_descriptor *vt, char c)
{
    vt_hide_cursor(vt);

    struct vt_state *state = &vt->state;
    struct vt_attr *attr = &vt->attr;

    if (vt->state.cursor_x > fb_get_width()-1) {
        vt_newline(vt);
    }

    if (c == '\033') {
        vt_escape_process(&vt->state.esc_state, c);
        return;
    }

    if (c == '\n') {
        vt_newline(vt);
        return;
    }
    
    if (VT_ESC_IS_PARSING(&vt->state.esc_state)) {
        if (vt_escape_process(&vt->state.esc_state, c) == 0) {
            /* Return if it is still parsing */
            return;
        }
    }
 
    vt_draw_char(vt, c, attr->text_fg, attr->text_bg);
    state->cursor_x += FONT_WIDTH;

    vt_show_cursor(vt);
}

void
vt_write(struct vt_descriptor *vt, const char *str, size_t len)
{
    spinlock_acquire(&vt->lock);

    for (size_t i = 0; i < len; ++i) {
        vt_putch(vt, str[i]);
    }

    spinlock_release(&vt->lock);
}

/*
 * Changes the specified virtual
 * terminal's attributes.
 *
 * @vt: Virtual terminal.
 * @attr: Attributes to use.
 */

int
vt_chattr(struct vt_descriptor *vt, const struct vt_attr *attr)
{
    /*
     * Verify cursor types.
     *
     * NOTE: Add all possible cursor types
     *       to this fall through.
     */
    switch (attr->cursor_type) {
        case CURSOR_NONE:
            break;
        case CURSOR_BLOCK:
            break;
        default:
            return -EINVAL;
    }
    
    if (attr != NULL) {
        vt->attr = *attr;
    } else {
        return -EINVAL;
    }

    return 0;
}


void
__vt_reset_unlocked(struct vt_descriptor *vt)
{
    vt_hide_cursor(vt);
    vt->state.cursor_x = 0;
    vt->state.cursor_y = 0;

    for (uint32_t cx = 0; cx < fb_get_width(); ++cx) {
        for (uint32_t cy = 0; cy < fb_get_height(); ++cy) {
            fb_put_pixel(vt->fb_base, cx, cy, vt->attr.bg);
        }
    }

    vt_show_cursor(vt);
}

/*
 * Resets the virtual terminal
 * state.
 *
 * @vt: Virtual terminal.
 */

void
vt_reset(struct vt_descriptor *vt)
{
    spinlock_acquire(&vt->lock);
    __vt_reset_unlocked(vt);
    spinlock_release(&vt->lock);
}

/*
 * Sets up the virtual terminal
 * state.
 *
 * @vt: Virtual terminal to set up.
 * @fb_base: Framebuffer base (set to NULL for default)
 * @attr: Initial attribute (set to NULL for default)
 *
 * NOTE: The default framebuffer base
 *       is the always visible framebuffer.
 */

int
vt_init(struct vt_descriptor *vt, const struct vt_attr *attr,
        uint32_t *fb_base)
{
    int status = 0;
    vt->fb_base = (fb_base == NULL) ? fb_get_base() : fb_base;
    vt->state.cursor_x = 0;
    vt->state.cursor_y = 0;
    vt->attr.bg = 0x000000;
    
    if (attr == NULL) {
        vt->attr.bg = DEFAULT_TERM_BG;
        vt->attr.text_fg = DEFAULT_TEXT_FG;
        vt->attr.text_bg = DEFAULT_TEXT_BG;
        vt->attr.cursor_bg = DEFAULT_CURSOR_BG;
        vt->attr.cursor_type = DEFAULT_CURSOR_TYPE;
    } else {
        status = vt_chattr(vt, attr);
    }

    vt_show_cursor(vt);
    vt_escape_init_state(&vt->state.esc_state, vt); 

    return status;
}

struct
vt_attr vt_getattr(struct vt_descriptor *vt)
{
    return vt->attr;
}

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

#ifndef _LIBGFX_DRAW_H_
#define _LIBGFX_DRAW_H_

#include <stdint.h>
#include <libgfx/gfx.h>

/* Shape types */
#define SHAPE_SQUARE  0x00000000

/* Basic color defines */
#define GFX_BLACK   0x000000
#define GFX_RED     0xFF0000
#define GFX_GREEN   0x00FF00
#define GFX_BLUE    0x0000FF
#define GFX_WHITE   0xFFFFFF
#define GFX_PURPLE  0x800080
#define GFX_YELLOW  0xFFFF00
#define GFX_DARK    0x1D2021
#define GFX_AQUA    0x427B58

/*
 * Default shape initializer, something that
 * works and can be tweaked. The idea of this
 * is so that shapes may be set up like so:
 *
 * --
 * struct gfx_shape blah = GFX_SHAPE_DEFAULT;
 *
 * blah.width = width;
 * blah.heiht = height;
 * ...
 * --
 */
#define GFX_SHAPE_DEFAULT       \
    {                           \
        .type = SHAPE_SQUARE,   \
        .color = 0x00FF00,      \
        .x = 0,                 \
        .y = 0,                 \
        .width = 50,            \
        .height = 50,           \
    }

/*
 * Generic shape representation
 *
 * @type: Shape type (see SHAPE_*)
 * @color: Color of the shape
 * @x: X position of the shape
 * @y: Y position of the shape
 * @width: Shape width
 * @height: Shape height
 */
struct gfx_shape {
    uint32_t type;
    color_t color;
    scrpos_t x;
    scrpos_t y;
    dimm_t width;
    dimm_t height;
};

/*
 * A point or single pixel that
 * may be plotted onto the screen.
 *
 * @x,y: Position of the point on the screen
 * @rgb: Color of the point (RGB)
 */
struct gfx_point {
    scrpos_t x, y;
    color_t rgb;
};

int gfx_draw_shape(struct gfx_ctx *ctx, const struct gfx_shape *shape);
int gfx_plot_point(struct gfx_ctx *ctx, const struct gfx_point *point);

__always_inline static inline size_t
gfx_io_index(struct gfx_ctx *ctx, scrpos_t x, scrpos_t y)
{
    struct fbattr fbdev = ctx->fbdev;

    return x + y * (fbdev.pitch  / 4);
}

#endif  /* !_LIBGFX_DRAW_H_ */

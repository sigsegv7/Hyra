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
#include <stdint.h>
#include <libgfx/gfx.h>
#include <libgfx/draw.h>

/*
 * Draw a classic square onto the screen.
 *
 * @ctx: Graphics context
 * @shape: Square to draw
 */
static int
gfx_draw_square(struct gfx_ctx *ctx, const struct gfx_shape *shape)
{
    struct fbattr fbdev;
    off_t idx;
    scrpos_t scr_width, scr_height;
    scrpos_t x, y;

    if (ctx == NULL || shape == NULL) {
        return -EINVAL;
    }

    /* Grab screen dimensions */
    fbdev = ctx->fbdev;
    scr_width = fbdev.width;
    scr_height = fbdev.height;

    for (x = shape->x; x < shape->x + shape->width; ++x) {
        for (y = shape->y; y < shape->y + shape->height; ++y) {
            if (x >= scr_width || y >= scr_height) {
                break;
            }

            idx = gfx_io_index(ctx, x, y);
            ctx->io[idx] = shape->color;
        }
    }
    return 0;
}

/*
 * Draw a shape onto the screen
 *
 * @ctx: libgfx graphics context
 * @shape: Shape to draw
 *
 * Returns 0 on success, otherwise a less than zero
 * value on failure.
 *
 * All error codes follow POSIX errno. However if the value
 * is -1, the requested shape to be drawn is not valid.
 */
int
gfx_draw_shape(struct gfx_ctx *ctx, const struct gfx_shape *shape)
{
    if (ctx == NULL || shape == NULL) {
        return -EINVAL;
    }

    switch (shape->type) {
    case SHAPE_SQUARE:
        return gfx_draw_square(ctx, shape);
    }

    return -1;
}

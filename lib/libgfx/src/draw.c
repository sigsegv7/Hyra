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
 * See if a pixel is within the bounds of
 * the screen.
 *
 * @ctx: Graphics context pointer
 * @x: X position to check
 * @y: Y position to check
 *
 * Returns 0 if within bounds, otherwise
 * a negative value.
 */
static int
gfx_pixel_bounds(struct gfx_ctx *ctx, uint32_t x, uint32_t y)
{
    scrpos_t scr_width, scr_height;
    struct fbattr fbdev;

    if (ctx == NULL) {
        return -1;
    }

    /* Grab screen dimensions */
    fbdev = ctx->fbdev;
    scr_width = fbdev.width;
    scr_height = fbdev.height;

    if (x >= scr_width || y >= scr_height) {
        return -1;
    }

    return 0;
}

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
    struct gfx_point p;
    off_t idx;
    scrpos_t x, y;

    if (ctx == NULL || shape == NULL) {
        return -EINVAL;
    }

    for (x = shape->x; x < shape->x + shape->width; ++x) {
        for (y = shape->y; y < shape->y + shape->height; ++y) {
            p.x = x;
            p.y = y;
            p.rgb = shape->color;
            gfx_plot_point(ctx, &p);
        }
    }
    return 0;
}

/*
 * Draw a bordered square onto the screen.
 *
 * @ctx: Graphics context pointer
 * @shape: Bordered square to draw
 */
static int
gfx_draw_bsquare(struct gfx_ctx *ctx, const struct gfx_shape *shape)
{
    struct gfx_point p;
    scrpos_t x_i, y_i;
    scrpos_t x_f, y_f;
    scrpos_t x, y;

    if (ctx == NULL || shape == NULL) {
        return -EINVAL;
    }

    x_i = shape->x;
    y_i = shape->y;
    x_f = shape->x + shape->width;
    y_f = shape->y + shape->height;

    /*
     * Draw an unfilled square.
     *
     * If we are at the `y_i' or `y_f' position, draw
     * pixels from 'x_i' to 'x_f'. If we are away
     * from the `y_i' position, draw two pixels,
     * one at `x_i' and the other at `x_f' for that
     * current 'y' value.
     */
    for (y = y_i; y < y_f; ++y) {
        for (x = x_i; x < x_f; ++x) {
            p.x = x;
            p.y = y;
            p.rgb = shape->color;

            /* Origin y, draw entire width */
            if (y == y_i || y == y_f - 1) {
                gfx_plot_point(ctx, &p);
                continue;
            }

            p.x = x_i;
            gfx_plot_point(ctx, &p);

            p.x = x_f - 1;
            gfx_plot_point(ctx, &p);
        }
    }

    return 0;
}

/*
 * Plot a single pixel (aka point) onto
 * the screen.
 *
 * @ctx: The graphics context pointer
 * @point: Point to plot
 *
 * Returns 0 on success, otherwise a less
 * than zero value.
 */
int
gfx_plot_point(struct gfx_ctx *ctx, const struct gfx_point *point)
{
    uint32_t index;

    if (ctx == NULL || point == NULL) {
        return -EINVAL;
    }

    /*
     * Is this even a valid point on the screen for
     * us to plot on?
     */
    if (gfx_pixel_bounds(ctx, point->x, point->y) < 0) {
        return -1;
    }

    /* Plot it !! */
    index = gfx_io_index(ctx, point->x, point->y);
    ctx->io[index] = point->rgb;
    return 0;
}

/*
 * Grab the RGB value of a single pixel on
 * the scren.
 *
 * @ctx: Graphics context pointer
 * @x: X position to sample
 * @y: Y position to sample
 */
color_t
gfx_get_pix(struct gfx_ctx *ctx, uint32_t x, uint32_t y)
{
    const color_t ERROR_COLOR = GFX_BLACK;
    uint32_t index;

    /* The 'ctx' argument is required */
    if (ctx == NULL) {
        return ERROR_COLOR;
    }

    /* Are we within bounds of the screen */
    if (gfx_pixel_bounds(ctx, x, y) < 0) {
        return ERROR_COLOR;
    }

    index = gfx_io_index(ctx, x, y);
    return ctx->io[index];
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
    case SHAPE_SQUARE_BORDER:
        return gfx_draw_bsquare(ctx, shape);
    }

    return -1;
}

/*
 * Copy a region on one part of a screen to
 * another part of a screen.
 *
 * @ctx: Graphics context pointer
 * @r: Region to copy
 * @x: X position for copy dest
 * @y: Y position for copy dest
 */
int
gfx_copy_region(struct gfx_ctx *ctx, struct gfx_region *r, scrpos_t x, scrpos_t y)
{
    struct gfx_point point;
    color_t pixel;
    scrpos_t src_cx, src_cy;
    dimm_t w, h;

    if (ctx == NULL || r == NULL) {
        return -EINVAL;
    }

    w = r->width;
    h = r->height;

    for (int xoff = 0; xoff < w; ++xoff) {
        for (int yoff = 0; yoff < h; ++yoff) {
            /* Source position */
            src_cx = r->x + xoff;
            src_cy = r->y + yoff;

            /* Plot the new pixel */
            pixel = gfx_get_pix(ctx, src_cx, src_cy);
            point.x = x + xoff;
            point.y = y + yoff;
            point.rgb = pixel;
            gfx_plot_point(ctx, &point);
        }
    }

    return 0;
}

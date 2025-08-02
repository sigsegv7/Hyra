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

#ifndef _LIBGFX_H_
#define _LIBGFX_H_

#include <sys/fbdev.h>
#include <stdint.h>
#include <stdio.h>

#define gfx_log(fmt, ...) printf( "libgfx: " fmt, ##__VA_ARGS__)

/*
 * Represents a 32-bit pixel value.
 *
 *   24:16  15:8  7:0
 *  +-----------------+
 *  | R  |   B   |  B |
 *  +-----------------+
 */
typedef uint32_t pixel_t;
typedef pixel_t color_t;

/*
 * Basic color defines
 */
#define GFX_BLACK   0x000000
#define GFX_RED     0xFF0000
#define GFX_GREEN   0x00FF00
#define GFX_BLUE    0x0000FF
#define GFX_WHITE   0xFFFFFF
#define GFX_PURPLE  0x800080
#define GFX_YELLOW  0xFFFF00

/*
 * Represents cartesian x/y values
 */
typedef uint32_t cartpos_t;
typedef cartpos_t scrpos_t;
typedef cartpos_t dimm_t;   /* Dimensions */

/*
 * Graphics context for libgfx
 *
 * @fbdev: Framebuffer attributes
 * @io: Framebuffer pointer
 * @fbfd: Framebuffer file descriptor
 */
struct gfx_ctx {
    struct fbattr fbdev;
    size_t fb_size;
    pixel_t *io;
    int fbfd;
};

int gfx_init(struct gfx_ctx *res);
void gfx_cleanup(struct gfx_ctx *ctx);

#endif  /* !_LIBGFX_H_ */

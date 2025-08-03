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

#ifndef LIBODA_ODA_H
#define LIBODA_ODA_H 1

#include <sys/queue.h>
#include <stdint.h>
#include <stddef.h>
#include <liboda/types.h>
#include <libgfx/gfx.h>
#include <libgfx/draw.h>

/*
 * ODA representation of a window.
 *
 * @surface: Window surface descriptor
 * @session: Session this window belongs to
 */
struct oda_window {
    struct gfx_shape surface;
    struct oda_state *session;
    TAILQ_ENTRY(oda_window) link;
};

/*
 * ODA session
 *
 * @winq: Window queue
 * @gctx: Graphics context
 * @cookie: State cookie (ODA_COOKIE)
 */
struct oda_state {
    TAILQ_HEAD(, oda_window) winq;
    struct gfx_ctx gctx;
    uint32_t cookie;
};

/*
 * ODA window attributes. Arguments to be
 * passed to oda_window_new()
 *
 * @session: Current ODA session / state
 * @parent: Window parent (NULL for root)
 * @pg: Background color (0xRRGGBB)
 * @x,y: Window position
 * @w,h: Window width [w] and height [h]
 */
struct oda_wattr {
    struct oda_state *session;
    struct oda_window *parent;
    odacolor_t bg;
    odapos_t x, y;
    odadimm_t w, h;
};

/*
 * A pixel point that can be plotted
 * onto a window.
 *
 * @x,y: Point position
 * @rgb: Color (RGB)
 * @window: Window this will be plotted to
 *
 * Just set x, y, the color (rgb) then point it
 * to a window!
 */
struct oda_point {
    odapos_t x, y;
    odacolor_t rgb;
    struct oda_window *window;
};

int oda_reqwin(struct oda_wattr *params, struct oda_window **res);
int oda_termwin(struct oda_state *state, struct oda_window *win);
int oda_plotwin(struct oda_state *state, const struct oda_point *point);

int oda_start_win(struct oda_state *state, struct oda_window *win);
int oda_init(struct oda_state *res);

#endif  /* !LIBODA_ODA_H */

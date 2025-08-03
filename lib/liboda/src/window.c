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
#include <stdlib.h>
#include <string.h>
#include <liboda/oda.h>
#include <liboda/odavar.h>
#include <liboda/types.h>
#include <libgfx/gfx.h>

/*
 * Request a window from the OSMORA Display
 * Architecture (ODA).
 *
 * @params: Arguments
 * @res: Resulting pointer for new window
 *
 * Returns 0 on success, otherwise a less than
 * zero value.
 */
int
oda_reqwin(struct oda_wattr *params, struct oda_window **res)
{
    struct oda_window *wp;
    struct gfx_shape *surf;
    struct oda_state *session;
    int error;

    if (params == NULL || res == NULL) {
        return -EINVAL;
    }

    /* Try to grab the current session */
    if ((session = params->session) == NULL) {
        return -EIO;
    }

    /* Verify that cookie! */
    if ((error = oda_cookie_verify(session)) != 0) {
        return error;
    }

    /* Allocate a new window */
    wp = malloc(sizeof(*wp));
    if (wp == NULL) {
        return -ENOMEM;
    }

    /* Initialize the window */
    memset(wp, 0, sizeof(*wp));
    wp->session = session;
    TAILQ_INSERT_TAIL(&session->winq, wp, link);

    /* Fix up width/height params */
    if (params->w == 0)
        params->w = DEFAULT_WIN_WIDTH;
    if (params->h == 0)
        params->h = DEFAULT_WIN_HEIGHT;

    /* Initialize the window surface */
    surf = &wp->surface;
    surf->color = params->bg;
    surf->x = params->x;
    surf->y = params->y;
    surf->width = params->w;
    surf->height = params->h;
    surf->type = SHAPE_SQUARE;
    *res = wp;
    return 0;
}

/*
 * Register a window into the current ODA state.
 * Everytime a compositor requests a window, we
 * must keep track of it.
 *
 * @state: ODA state pointer
 * @win: Pointer of window to register
 */
int
oda_start_win(struct oda_state *state, struct oda_window *win)
{
    int error;

    if (state == NULL || win == NULL) {
        return -EINVAL;
    }

    /* Make sure the state is valid */
    if ((error = oda_cookie_verify(state)) != 0) {
        return error;
    }

    gfx_draw_shape(&state->gctx, &win->surface);
    return 0;
}

/*
 * Terminate a running window
 *
 * @state: ODA state pointer
 * @win: Win pointer
 *
 * Returns 0 on success, otherwise a less than
 * zero value.
 *
 * TODO: Cleanup screen
 */
int
oda_termwin(struct oda_state *state, struct oda_window *win)
{
    int error;

    if (state == NULL || win == NULL) {
        return -EINVAL;
    }

    /* Validate the cookie */
    if ((error = oda_cookie_verify(state)) != 0) {
        return error;
    }

    TAILQ_REMOVE(&state->winq, win, link);
    free(win);
    return 0;
}

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
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>
#include <liboda/oda.h>
#include <liboda/odavar.h>
#include <liboda/types.h>
#include <libgfx/gfx.h>

/*
 * The window cache is used to reduce how many
 * calls to malloc() and free() are made during
 * window creation and destruction.
 */
static TAILQ_HEAD(, oda_window) wcache;
static uint32_t wcache_cookie = 0;
static odawid_t next_wid = 1;

/*
 * Pop a window from the window cache.
 * Returns NULL there are no more windows.
 */
static struct oda_window *
oda_window_pop(void)
{
    struct oda_window *wdp;

    if (wcache_cookie != ODA_COOKIE) {
        TAILQ_INIT(&wcache);
        wcache_cookie = ODA_COOKIE;
        return NULL;
    }

    wdp = TAILQ_FIRST(&wcache);
    TAILQ_REMOVE(&wcache, wdp, link);
    return wdp;
}

/*
 * Place a window into the window cache.
 */
static void
oda_window_cache(struct oda_window *wdp)
{
    /* Ensure arg is valid */
    if (wdp == NULL) {
        return;
    }

    if (wcache_cookie != ODA_COOKIE) {
        TAILQ_INIT(&wcache);
        wcache_cookie = ODA_COOKIE;
    }

    TAILQ_INSERT_TAIL(&wcache, wdp, link);
}

/*
 * Allocate an ODP window
 *
 * Returns NULL on failure
 */
static struct oda_window *
oda_window_alloc(void)
{
    struct oda_window *wdp;

    /*
     * First check if there are any entries
     * we can grab from the window cache.
     */
    wdp = oda_window_pop();
    if (wdp != NULL) {
        return wdp;
    }

    /* Allocate a new window */
    wdp = malloc(sizeof(*wdp));
    if (wdp == NULL) {
        return NULL;
    }

    memset(wdp, 0, sizeof(*wdp));
    wdp->wid = next_wid++;
    return wdp;
}

/*
 * Release a given ODA window descriptor
 *
 * @wdp: Window to free
 */
static void
oda_window_release(struct oda_state *state, struct oda_window *wdp)
{
    if (wdp == NULL) {
        return;
    }

    /*
     * It is probably a good idea to ensure previous
     * state other than the old window ID is reset
     * and zeroed.
     */
    wdp->session = NULL;
    memset(&wdp->surface, 0, sizeof(wdp->surface));

    /*
     * Now we can remove this window from the list
     * of windows we are tracking and add it to the
     * cache.
     */
    TAILQ_REMOVE(&state->winq, wdp, link);
    oda_window_cache(wdp);
}

/*
 * Check if a point is within the bounds of
 * a surface.
 *
 * @wp: Surface to check with point
 * @point: Point to check with surface
 *
 * Returns 0 if the check has passed,
 * otherwise a less than zero value.
 */
static int
oda_check_point(struct oda_window *wp, struct oda_point *point)
{
    struct gfx_shape *surf;
    scrpos_t win_startx;
    scrpos_t win_starty;
    scrpos_t win_endx;
    scrpos_t win_endy;

    /* Compute start positions */
    surf = &wp->surface;
    win_startx = surf->x;
    win_starty = surf->y;

    /* Compute end positions */
    win_endx = surf->x + surf->width;
    win_endy = surf->y + surf->height;

    /* Check X bounds */
    if (point->x < win_startx || point->x > win_endx) {
        return -1;
    }

    /* Check Y bounds */
    if (point->y < win_starty || point->y > win_endy) {
        return -1;
    }

    /* All good */
    return 0;
}

/*
 * Clean up after ourselves and release
 * each entry of the wcache.
 *
 * Returns 0 on success.
 */
static int
oda_free_wcache(void)
{
    struct oda_window *wdp, *next;

    if (wcache_cookie != ODA_COOKIE) {
        return -1;
    }

    /*
     * Go through each entry and call free()
     * on them.
     */
    wdp = TAILQ_FIRST(&wcache);
    while (wdp != NULL) {
        next = TAILQ_NEXT(wdp, link);
        free(wdp);
        wdp = next;
    }
    return 0;
}

/*
 * Plot a pixel onto a window
 *
 * @state: ODA state pointer
 * @point: Point to plot
 *
 * Returns 0 on success, otherwise a less than
 * zero value.
 *
 * XXX: The x/y params in the 'point' argument must be
 *      relative to the start of the window. In other words,
 *      (0,0) refers to the top left corner of the window.
 */
int
oda_plotwin(struct oda_state *state, const struct oda_point *point)
{
    struct gfx_point pixel;
    struct oda_point point_new;
    struct oda_window *window;
    struct gfx_shape *surf;
    odapos_t plotx, ploty;
    int error;

    if (state == NULL || point == NULL) {
        return -EINVAL;
    }

    /* Validate cookie */
    if ((error = oda_cookie_verify(state)) != 0) {
        return error;
    }

    /* Try to grab the window */
    if ((window = point->window) == NULL) {
        return -EINVAL;
    }

    surf = &window->surface;
    plotx = surf->x + point->x;
    ploty = surf->y + point->y;

    /*
     * We are going to need to transform the coordinates
     * as they are supposed to be coming in relative to
     * the window bounds, e.g., (0,0) being the top left
     * corner of a window.
     */
    point_new = *point;
    point_new.x = plotx;
    point_new.y = ploty;

    /* Initialize the pixel to plot */
    pixel.x = plotx;
    pixel.y = ploty;
    pixel.rgb = point->rgb;

    /* Is the point within bounds? */
    error = oda_check_point(window, &point_new);
    if (error < 0) {
        return error;
    }

    gfx_plot_point(&state->gctx, &pixel);
    return 0;
}

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
    wp = oda_window_alloc();
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

    oda_window_release(state, win);
    return 0;
}

/*
 * Shutdown the ODA library
 */
int
oda_shutdown(struct oda_state *state)
{
    return oda_free_wcache();
}

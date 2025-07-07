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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/console.h>
#include <dev/cons/cons.h>
#include <dev/cons/ansi.h>
#include <string.h>

__always_inline static inline bool
is_valid_color(int c)
{
    return c >= '0' && c <= '7';
}

static inline void
ansi_reset(struct ansi_state *statep)
{
    memset(statep, 0, sizeof(*statep));
}

/*
 * Feed a byte into the ANSI escape sequence
 * state machine.
 *
 * @statep: State machine pointer.
 * @c: Byte to feed.
 *
 * On success, `c' is returned. On failure,
 * 0 is returned. Values less than 0 indicate
 * success with console attributes updated
 * (ANSI_UPDATE_*).
 */
int
ansi_feed(struct ansi_state *statep, char c)
{
    struct cons_screen *scr = &g_root_scr;
    struct console_feat *featp;


    /* Standard colors */
    static uint32_t colortab[] = {
        ANSI_BLACK, ANSI_RED,
        ANSI_GREEN, ANSI_YELLOW,
        ANSI_BLUE, ANSI_MAGENTA,
        ANSI_CYAN, ANSI_WHITE
    };

    featp = &scr->feat;
    if (!featp->ansi_esc) {
        return 0;
    }

    /*
     * Handle the control sequence introducer
     * bytes.
     */
    switch (statep->csi) {
    case 0: /* '\033' */
        if (c != '\033') {
            return 0;
        }
        statep->csi = 1;
        statep->prev = c;
        return c;
    case 1: /* '[' */
        if (c != '[') {
            ansi_reset(statep);
            return 0;
        }
        statep->csi = 2;
        statep->prev = c;
        return c;
    case 2:
        if (c == '2') {
            statep->csi = 3;
            statep->prev = c;
            return c;
        }
        break;
    case 3:
        /* Did we get '\033[2J' ? */
        if (statep->prev == '2' && c == 'J') {
            cons_clear_scr(scr, g_root_scr.bg);
            ansi_reset(statep);
            return ANSI_UPDATE_CURSOR;
        }
        break;
    }

    if (!statep->set_fg && !statep->set_bg) {
        /* Reset attributes? */
        if (statep->reset_color) {
            ansi_reset(statep);
            cons_reset_color(scr);
            return ANSI_UPDATE_COLOR;
        }

        /* Mark attributes to be reset? */
        if (c == '0') {
            statep->reset_color = 1;
            statep->prev = c;
            return c;
        }

        /* Expect foreground */
        if (c != '3') {
            ansi_reset(statep);
            return 0;
        }
        statep->set_fg = 1;
        statep->prev = c;
        return c;
    }

    if (statep->set_fg && c != ';') {
        /* Make sure this is valid */
        if (!is_valid_color(c)) {
            ansi_reset(statep);
            return 0;
        }

        /* Set the foreground */
        statep->fg = colortab[c - '0'];
        statep->set_bg = 1;
        statep->set_fg = 0;
        statep->prev = c;
        return c;
    }

    if (statep->set_bg) {
        if (c == ';') {
            statep->prev = c;
            return c;
        }

        /* Expect '4' after ';' */
        if (statep->prev == ';' && c != '4') {
            ansi_reset(statep);
            return 0;
        }

        if (c == 'm') {
            cons_update_color(scr, statep->fg, statep->bg);
            ansi_reset(statep);
            return ANSI_UPDATE_COLOR;
        }

        /* Make sure this is valid */
        if (!is_valid_color(c)) {
            ansi_reset(statep);
            return 0;
        }

        /* Set the background */
        statep->bg = colortab[c - '0'];
        statep->prev = c;
        return c;
    }

    ansi_reset(statep);
    return 0;
}

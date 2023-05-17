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

#include <vt/escape.h>
#include <sys/types.h>
#include <vt/vt.h>

static inline bool
vt_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static void
vt_try_set_color(struct vt_escape_state *state)
{
    if (state->fg == VT_COLOR_NONE || state->bg == VT_COLOR_NONE) {
        return;
    }

    uint32_t lookup_table[] = {
        [VT_COLOR_BLACK]    = 0x000000,
        [VT_COLOR_RED]      = 0xFF0000,
        [VT_COLOR_GREEN]    = 0x50C878,
        [VT_COLOR_YELLOW]   = 0xFFD700,
        [VT_COLOR_BLUE]     = 0x6495ED,
        [VT_COLOR_MAGENTA]  = 0xFF00FF
    };

    struct vt_attr vtattr = vt_getattr(state->vt);

    if (state->fg == VT_COLOR_RESET)
        vtattr.text_fg = DEFAULT_TEXT_FG;
    if (state->bg == VT_COLOR_RESET)
        vtattr.text_fg = DEFAULT_TEXT_FG;
    if (state->fg != VT_COLOR_RESET)
        vtattr.text_fg = lookup_table[state->fg];
    if (state->bg != VT_COLOR_RESET)
        vtattr.text_bg = lookup_table[state->bg];

    vt_chattr(state->vt, &vtattr);
}

void
vt_escape_init_state(struct vt_escape_state *state, struct vt_descriptor *vt)
{
    state->status = VT_PARSE_AWAIT;
    state->fg = VT_COLOR_NONE;
    state->bg = VT_COLOR_NONE;
    state->vt = vt;
}

int
vt_escape_process(struct vt_escape_state *state, char c)
{
    int status = 0;

    switch (state->status) {
    case VT_PARSE_AWAIT: 
        if (c == '\033') {
            state->status = VT_PARSE_ESC;
            status = 1;
        }
        break;
    case VT_PARSE_ESC:          /* '\033' */
        if (c == '[') {
            state->status = VT_PARSE_BRACKET;
        } else {
            state->status = VT_PARSE_AWAIT;
            status = 1;
        }
        
        break;
    case VT_PARSE_BRACKET:
        if (vt_is_digit(c)) {
            state->status = VT_PARSE_DIGIT;
            state->last_digit = c;
        } else {
            /* TODO: Perhaps support stuff like '\033[m'? */
            state->status = VT_PARSE_AWAIT;
            status = 1;
        }
        break;
    case VT_PARSE_DIGIT:
        if (state->last_digit == '2' && c == 'J') {
            /* '\033[2J': Clear screen */
            __vt_reset_unlocked(state->vt);
            state->status = VT_PARSE_AWAIT;
        } else if (state->last_digit == '0' && c == 'm') {
            /* '\033[0m': Reset colors */
            state->fg = VT_COLOR_RESET;
            state->bg = VT_COLOR_RESET; 

            vt_try_set_color(state);
            state->status = VT_PARSE_AWAIT;
        } else if (state->last_digit == '3' && c == '0') {
            /* FG: 30 (black) */
            state->fg = VT_COLOR_BLACK;
        } else if (state->last_digit == '3' && c == '1') {
            state->fg = VT_COLOR_RED;
        } else if (state->last_digit == '3' && c == '2') {
            state->fg = VT_COLOR_GREEN;
        } else if (state->last_digit == '3' && c == '3') {
            state->fg = VT_COLOR_YELLOW;
        } else if (state->last_digit == '3' && c == '4') {
            state->fg = VT_COLOR_BLUE;
        } else if (state->last_digit == '3' && c == '5') {
            state->fg = VT_COLOR_MAGENTA;
        } else if (c == ';') {
            /* Time to parse the background color */
            state->status = VT_PARSE_BACKGROUND;
            state->last_digit = '\0';
        } else if (c == 'm') {
            /* No background */
            state->status = VT_PARSE_AWAIT;
            state->bg = VT_COLOR_RESET;
            vt_try_set_color(state);
        } else {
            state->status = VT_PARSE_AWAIT;
        }
        break;
    case VT_PARSE_BACKGROUND:
        if (state->last_digit == '\0' && vt_is_digit(c)) {
            state->last_digit = c;
        } else if (state->last_digit == '4' && c == '0') {
            state->bg = VT_COLOR_BLACK;
        } else if (state->last_digit == '4' && c == '1') {
            state->bg = VT_COLOR_RED;
        } else if (state->last_digit == '4' && c == '2') {
            state->bg = VT_COLOR_GREEN;
        } else if (state->last_digit == '4' && c == '3') {
            state->bg = VT_COLOR_YELLOW;
        } else if (state->last_digit == '4' && c == '4') {
            state->bg = VT_COLOR_BLUE;
        } else if (state->last_digit == '4' && c == '5') {
            state->bg = VT_COLOR_MAGENTA;
        } else if (c == 'm') {
            vt_try_set_color(state);
            state->status = VT_PARSE_AWAIT;
        } else {
            state->status = VT_PARSE_AWAIT;
        }

        break;
    }

    return status;
}

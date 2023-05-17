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

#ifndef _VT_ESCAPE_H_
#define _VT_ESCAPE_H_

#define VT_ESC_IS_PARSING(esc_state_ptr) \
    ((esc_state_ptr)->status != VT_PARSE_AWAIT)

struct vt_descriptor;

typedef enum {
    VT_COLOR_NONE,
    VT_COLOR_RESET,
    VT_COLOR_BLACK,
    VT_COLOR_RED,
    VT_COLOR_GREEN,
    VT_COLOR_YELLOW,
    VT_COLOR_BLUE,
    VT_COLOR_MAGENTA,
} vt_color_t;

/*
 * State machine for escape
 * code parsing.
 */

struct vt_escape_state {
    enum {
        VT_PARSE_AWAIT,
        VT_PARSE_ESC,
        VT_PARSE_BRACKET,
        VT_PARSE_DIGIT,
        VT_PARSE_BACKGROUND
    } status; 
    
    vt_color_t fg;
    vt_color_t bg;
    char last_digit;
    struct vt_descriptor *vt;
};

int vt_escape_process(struct vt_escape_state *state, char c);
void vt_escape_init_state(struct vt_escape_state *state, struct vt_descriptor *vt);

#endif

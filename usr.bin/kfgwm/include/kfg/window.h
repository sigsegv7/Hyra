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

#ifndef KFG_WINDOW_H_
#define KFG_WINDOW_H_

#include <kfg/types.h>

#define KFG_RED 0x6E0C24
#define KFG_YELLOW 0xF0A401
#define KFG_WHITE 0xF2E5BC
#define KFG_DARK  0x1D2021
#define KFG_BLUE  0x076678
#define KFG_AQUA  0x427B58

/* Default dimensions */
#define KFG_BORDER_WIDTH 1
#define KFG_BORDER_HEIGHT 1
#define KFG_TITLE_HEIGHT 10

struct kfg_window {
    kfgpos_t x;
    kfgpos_t y;
    kfgdim_t width;
    kfgdim_t height;
    kfgdim_t fb_pitch;
    kfgpixel_t bg;
    kfgpixel_t border_bg;
    kfgpixel_t *framebuf;
};

struct kfg_text {
    const char *text;
    kfgpos_t x;
    kfgpos_t y;
};

int kfg_win_draw(struct kfg_window *parent, struct kfg_window *wp);
int kfg_win_putstr(struct kfg_window *wp, struct kfg_text *tp);

#endif  /* !KFG_WINDOW_H_ */

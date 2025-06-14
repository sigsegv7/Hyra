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

#ifndef _DEV_CONS_H_
#define _DEV_CONS_H_

#include <sys/types.h>
#include <sys/spinlock.h>
#include <dev/video/fbdev.h>
#include <dev/cons/consvar.h>
#include <dev/cons/ansi.h>

struct cons_char {
    char c;
    uint32_t fg;
    uint32_t bg;
    uint32_t x;
    uint32_t y;
};

struct cons_screen {
    struct fbdev fbdev;
    struct ansi_state ansi_s;
    uint32_t fg;
    uint32_t bg;

    /* Private */
    uint32_t *fb_mem;
    uint32_t nrows;
    uint32_t ncols;
    uint32_t ch_col;    /* Current col */
    uint32_t ch_row;    /* Current row */
    uint32_t curs_col;  /* Cursor col */
    uint32_t curs_row;  /* Cursor row */
    struct cons_buf *ib;  /* Input buffer */
    struct cons_buf **ob; /* Output buffers */
    struct cons_char last_chr;
    struct spinlock lock;
};

void cons_init(void);
void cons_expose(void);
void cons_update_color(struct cons_screen *scr, uint32_t fg, uint32_t bg);
void cons_clear_scr(struct cons_screen *scr, uint32_t bg);
void cons_reset_color(struct cons_screen *scr);
void cons_reset_cursor(struct cons_screen *scr);
int cons_putch(struct cons_screen *scr, char c);
int cons_putstr(struct cons_screen *scr, const char *s, size_t len);

extern struct cons_screen g_root_scr;

#endif  /* !_DEV_CONS_H_ */

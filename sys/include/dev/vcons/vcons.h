/*
 * Copyright (c) 2023-2024 Ian Marco Moffett and the Osmora Team.
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

#ifndef _DEV_VCONS_H_
#define _DEV_VCONS_H_

#include <sys/types.h>
#include <sys/termios.h>
#include <dev/video/fbdev.h>
#include <sysfont.h>

#define VCONS_TAB_WIDTH 4
#define VCONS_CURSOR_WIDTH FONT_WIDTH
#define VCONS_CURSOR_HEIGHT FONT_HEIGHT

struct vcons_cursor {
    size_t xpos, ypos;
    uint32_t rgb;

    /* Internal */
    uint32_t old_xpos, old_ypos;
    volatile bool is_drawing;
    volatile bool is_drawn;     /* If it has been drawn before */
};

struct vcons_screen {
    size_t nrows, ncols;
    size_t cpy_x, cpy_y;    /* In chars */
    size_t cpy_len;

    uint32_t bg;
    uint32_t fg;
    void *fbdev_mem;

    struct fbdev fbdev;
    struct vcons_cursor cursor;
    struct termios termios;
};

#define is_cursor_drawing(screenptr) (screenptr)->cursor.is_drawing

void vcons_attach(struct vcons_screen *scr);
int vcons_putch(struct vcons_screen *scr, char c);
int vcons_putstr(struct vcons_screen *scr, const char *s);
void vcons_update_cursor(struct vcons_screen *scr);

#endif  /* !_DEV_VCONS_H_ */

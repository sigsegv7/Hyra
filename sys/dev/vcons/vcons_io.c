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

#include <dev/vcons/vcons_io.h>
#include <dev/vcons/vcons.h>
#include <sys/cdefs.h>
#include <sys/ascii.h>

static void
vcons_expand_tab(struct vcons_screen *scr)
{
    for (size_t i = 0; i < VCONS_TAB_WIDTH; ++i) {
        vcons_putch(scr, ' ');
    }
}

/*
 * This routine tries to process the output `c'.
 *
 * Returns < 0 value on failure. Values >= 0
 * is `c' which may differ from the original.
 *
 * This routine also may modify the screen state
 * if `c' is a control character.
 */
int
vcons_process_output(struct vcons_screen *scr, int c)
{
    struct vcons_cursor *cursor = &scr->cursor;

    switch (c) {
    case ASCII_LF:
        scr->cpy_y++;
        cursor->ypos += FONT_HEIGHT;

        scr->cpy_x = 0;
        cursor->xpos = 0;
        break;
    case ASCII_CR:
        scr->cpy_x = 0;
        cursor->xpos = 0;
        break;
    case ASCII_HT:
        vcons_expand_tab(scr);
        break;
    default:
        return -1;
    }

    vcons_update_cursor(scr);
    return c;
}

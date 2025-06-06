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

#ifndef _CONS_ANSI_H_
#define _CONS_ANSI_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>

/* ANSI colors */
#define ANSI_BLACK      0x000000
#define ANSI_RED        0xAA0000
#define ANSI_GREEN      0x00AA00
#define ANSI_BLUE       0x0000AA
#define ANSI_YELLOW     0xAA5500
#define ANSI_MAGENTA    0xAA00AA
#define ANSI_CYAN       0x00AAAA
#define ANSI_WHITE      0xAAAAAA

/* ANSI_FEED update codes */
#define ANSI_UPDATE_COLOR  -1

/*
 * ANSI parser state machine.
 *
 * @prev: Previous char
 * @csi: Encountered control seq introducer
 * @reset_color: 1 if color is to be reset
 * @set_fg: 1 if fg is being set
 * @set_bg: 1 if bg is being set
 * @fg: Foreground color
 * @bg: Background color
 * @flags: State flags
 */
struct ansi_state {
    char prev;
    uint8_t csi : 2;
    uint8_t reset_color : 1;
    uint8_t set_fg : 1;
    uint8_t set_bg : 1;
    uint32_t fg;
    uint32_t bg;
};

int ansi_feed(struct ansi_state *statep, char c);

#endif  /* !_CONS_ANSI_H_ */

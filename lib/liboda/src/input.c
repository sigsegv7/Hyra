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

#include <sys/ascii.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <liboda/oda.h>
#include <liboda/input.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Convert key scancode/char values to fixed
 * ODA key constants
 */
static inline uint16_t
oda_map_key(const struct oda_key *key)
{
    uint16_t type = ODA_KEY_OTHER;

    switch (key->ch) {
    case ASCII_ESC:
        type = ODA_KEY_ESCAPE;
        break;
    case ASCII_HT:
        type = ODA_KEY_TAB;
        break;
    case ASCII_BS:
        type = ODA_KEY_BACKSPACE;
        break;
    }

    return type;
}

/*
 * Dispatch keyboard events. This is typically
 * called in an event loop so that keyboard events
 * are handled per iteration.
 *
 * @kbd: Keyboard to monitor
 */
int
oda_kbd_dispatch(struct oda_kbd *kbd)
{
    struct oda_key key;
    int input;

    if (kbd == NULL) {
        return -EINVAL;
    }

    /* Attempt to grab the input */
    if ((input = getchar()) < 0) {
        return -EAGAIN;
    }

    key.scancode = ODA_SCANCODE(input);
    key.ch = ODA_KEYCHAR(input);
    key.type = oda_map_key(&key);
    return kbd->handle_keyev(kbd, &key);
}

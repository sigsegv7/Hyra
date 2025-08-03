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

#ifndef LIBODA_INPUT_H
#define LIBODA_INPUT_H

#include <stdint.h>
#include <stddef.h>

/*
 * Macros to help extract scancode and
 * character
 */
#define ODA_SCANCODE(KEY) ((KEY) >> 8)
#define ODA_KEYCHAR(KEY) ((char )(KEY) & 0xFF)

/*
 * Key defines
 */
#define ODA_KEY_OTHER       0x0000
#define ODA_KEY_ESCAPE      0x0001
#define ODA_KEY_TAB         0x0002
#define ODA_KEY_BACKSPACE   0x0003

/*
 * Represents a key press
 *
 * @type: Key types (see ODA_KEY_*)
 * @scancode: Scancode
 * @ch: Character
 */
struct oda_key {
    uint16_t type;
    uint8_t scancode;
    char ch;
};

/*
 * ODA keyboard object for managing keyboard
 * input.
 */
struct oda_kbd {
    int(*handle_keyev)(struct oda_kbd *kbd, struct oda_key *key);
};

int oda_kbd_dispatch(struct oda_kbd *kbd);

#endif  /* !LIBODA_INPUT_H */

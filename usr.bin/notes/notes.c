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

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define BEEP_MSEC 100
#define key_step(KEY) ('9' - ((KEY)))

static uint8_t freq_addend = 0;

static uint16_t freqtab[] = {
    [ key_step('0') ] = 950,
    [ key_step('9') ] = 900,
    [ key_step('8') ] = 850,
    [ key_step('7') ] = 800,
    [ key_step('6') ] = 750,
    [ key_step('5') ] = 700,
    [ key_step('4') ] = 650,
    [ key_step('3') ] = 600,
    [ key_step('2') ] = 550,
    [ key_step('1') ] = 500
};

static int beep_fd = 0;
static bool running = false;

static void
beep(uint16_t freq)
{
    uint32_t payload;

    if (beep_fd < 0) {
        return;
    }

    payload = freq;
    payload |= (BEEP_MSEC << 16);
    write(beep_fd, &payload, sizeof(payload));
}

static inline void
play_notekey(char key)
{
    uint8_t step = key_step(key);
    uint16_t freq;

    /* Should not happen */
    if (step >= NELEM(freqtab)) {
        step = key_step('0');
    }

    freq = freqtab[step] + freq_addend;
    beep(freq);
}

static void
play_loop(void)
{
    uint16_t freq = 0;
    char c;

    running = true;
    while (running) {
        c = getchar();
        switch (c) {
        case 'q':
            running = false;
            break;
        case 'i':
            /* NOTE: Overflow purposefully allowed here */
            ++freq_addend;
            printf("%d ", freq_addend);
            break;
        case 'd':
            /* NOTE: Underflow purposefully allowed here */
            --freq_addend;
            printf("%d ", freq_addend);
            break;
        default:
            if (!isdigit(c)) {
                break;
            }

            play_notekey(c);
        }
    }

    printf("\ncya!\n");
}

int
main(int argc, char **argv)
{
    beep_fd = open("/dev/beep", O_WRONLY);
    if (beep_fd < 0) {
        return -1;
    }

    printf("bleep bloop time! - [i]nc/[d]ec\n");
    play_loop();
    close(beep_fd);
}

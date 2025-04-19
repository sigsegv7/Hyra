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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#define prcons(FD, STR) write((FD), (STR), strlen((STR)))
#define is_ascii(C) ((C) >= 0 && (C) <= 128)
#define WELCOME \
    ":::::::::::::::::::::::::::::::::::::::\n" \
    ":: OSMORA GATEWAY ~ Every key echos  ::\n" \
    ":: ..... Proceed with purpose .....  ::\n" \
    ":::::::::::::::::::::::::::::::::::::::\n"

#define CMD_ECHO "echo"

static char buf[64];
static uint8_t i;

static void
cmd_run(int fd)
{
    int cmp;
    char *p = buf;
    size_t len;
    int valid_cmd = 1;

    switch (buf[0]) {
    case 'e':
        len = strlen(CMD_ECHO);
        if (memcmp(buf, CMD_ECHO, len) == 0) {
            p += len + 1;
            prcons(fd, "\n");
            prcons(fd, p);
            break;
        }

        valid_cmd = 0;
        break;
    default:
        valid_cmd = 0;
        break;
    }

    if (!valid_cmd) {
        prcons(fd, "\nunrecognized command\n");
    }
}

int
main(int argc, char **argv)
{
    int fd;
    uint16_t input;
    char c;

    if ((fd = open("/dev/console", O_RDWR)) < 0) {
        return fd;
    }

    i = 0;
    prcons(fd, WELCOME);
    prcons(fd, "[root::osmora]~ ");
    for (;;) {
        if (read(fd, &input, 2) <= 0) {
            continue;
        }

        c = input & 0xFF;
        if (!is_ascii(c)) {
            continue;
        }

        if (i < sizeof(buf)) {
            buf[i++] = c;
            buf[i] = 0;
        }
        if (c == '\n') {
            cmd_run(fd);
            i = 0;
            buf[i] = 0;
            prcons(fd, "[root::osmora]~ ");
        } else {
            write(fd, &c, 1);
        }
    }
    return 0;
}

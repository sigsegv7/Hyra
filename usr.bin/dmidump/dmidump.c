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

#include <sys/dmi.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

/*
 * The kernel fills DMI structures to zero,
 * if any of the fields are unset then p[0]
 * will have a null terminator which tells
 * us we should ignore it.
 */
static void
dmi_printfield(const char *name, const char *p)
{
    if (p[0] == '\0') {
        return;
    }

    printf("%s: %s\n", name, p);
}

static void
dmi_dump_board(void)
{
    struct dmi_board board;
    int fd;

    fd = open("/ctl/dmi/board", O_RDONLY);
    if (fd < 0) {
        printf("failed to open board control\n");
        return;
    }

    read(fd, &board, sizeof(board));
    printf("** BOARD INFO **\n");
    dmi_printfield("CPU version", board.cpu_version);
    dmi_printfield("CPU OEM", board.cpu_manuf);
    dmi_printfield("product", board.product);
    dmi_printfield("vendor", board.vendor);
    dmi_printfield("version", board.version);
    close(fd);
}

int
main(int argc, char **argv)
{
    dmi_dump_board();
    return 0;
}

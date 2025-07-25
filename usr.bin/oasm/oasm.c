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
#include <fcntl.h>
#include <unistd.h>
#include <oasm/state.h>
#include <oasm/parse.h>
#define OASM_DBG
#include <oasm/log.h>

struct oasm_state g_state;

static void
oasm_start(struct oasm_state *state)
{
    state->line = 1;
    parse_enter(state);
}

int
main(int argc, char **argv)
{
    if (argc < 3) {
        printf("oasm: usage: oasm <file> <output>\n");
        return -1;
    }

    g_state.in_fd = open(argv[1], O_RDONLY);
    if (g_state.in_fd < 0) {
        printf("could not open \"%s\"\n", argv[1]);
        return -1;
    }

    g_state.out_fd = open(argv[2], O_CREAT | O_WRONLY);
    if (g_state.out_fd < 0) {
        printf("could not open output \"%s\"\n", argv[2]);
        close(g_state.in_fd);
        return -1;
    }

    g_state.filename = argv[1];
    oasm_start(&g_state);
    close(g_state.in_fd);
    close(g_state.out_fd);
    return 0;
}

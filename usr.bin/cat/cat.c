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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_MODE_NONE       0
#define NUM_MODE_ALL        1
#define NUM_MODE_NONBLANK   2

static void
help(void)
{
    printf(
        "usage: cat <flags> <file>\n"
        "[-b]   do not number blank lines\n"
        "[-n]   number all lines\n"
    );
}

static void
cat(const char *pathname, int num_mode)
{
    FILE *file;
    char buf[64];
    int fd;
    size_t lineno = 1;

    file = fopen(pathname, "r");
    if (file == NULL) {
        return;
    }

    while (fgets(buf, sizeof(buf), file) != NULL) {
        switch (num_mode) {
        case NUM_MODE_NONE:
            break;
        case NUM_MODE_ALL:
            printf("%d   ", lineno);
            break;
        case NUM_MODE_NONBLANK:
            if (buf[0] == '\n') {
                break;
            }
            printf("%d   ", lineno);
            break;
        }
        printf("%s", buf);
        ++lineno;
    }

    fclose(file);
}

int
main(int argc, char **argv)
{
    int num_mode = NUM_MODE_NONE;
    int c;

    if (argc < 2) {
        help();
        return -1;
    }

    while ((c = getopt(argc, argv, "nb")) != -1) {
        switch (c) {
        case 'n':
            num_mode = NUM_MODE_ALL;
            break;
        case 'b':
            num_mode = NUM_MODE_NONBLANK;
            break;
        }
    }

    for (size_t i = optind; i < argc; ++i) {
        cat(argv[i], num_mode);
    }

    return 0;
}

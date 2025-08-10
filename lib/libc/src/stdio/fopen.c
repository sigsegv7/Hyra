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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

FILE *
fopen(const char *__restrict path, const char *__restrict mode)
{
    FILE *fhand;
    int fd;
    mode_t seal = 0;

    if (path == NULL || mode == NULL) {
        return NULL;
    }

    /* Create the seal from mode string */
    if (strcmp(mode, "r") == 0) {
        seal |= O_RDONLY;
    } else if (strcmp(mode, "w") == 0) {
        seal |= (O_WRONLY | O_CREAT);
    } else if (strcmp(mode, "r+") == 0) {
        seal |= O_RDWR;
    } else if (strcmp(mode, "rb") == 0) {
        seal |= O_RDONLY;
    } else {
        return NULL;
    }

    /* Try to open the file descriptor */
    fd = open(path, seal);
    if (fd < 0) {
        return NULL;
    }

    fhand = malloc(sizeof(*fhand));
    if (fhand == NULL) {
        return NULL;
    }

    fhand->fd = fd;
    fhand->buf_mode = _IONBF;
    return fhand;
}

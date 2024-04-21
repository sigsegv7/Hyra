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

#include <stdio.h>

#include <unistd.h>

#define WRITE_BUF_SIZE 128

/* TODO: Allocate these once malloc() is implemented */
static FILE default_stream;
static char default_write_buf[WRITE_BUF_SIZE];

FILE*
fdopen(int fd, const char *mode)
{
    FILE *stream;
    int flags;

    if (mode == NULL)
        return NULL;

    /* Determine stream flags */
    flags = 0;
    switch (mode[0]) {
    case 'r':
        flags |= FILE_READ;
        if (mode[1] == '+') {
            flags |= FILE_WRITE;
        }
        break;
    case 'w':
    case 'a':
        flags |= FILE_WRITE;
        if (mode[1] == '+') {
            flags |= FILE_READ;
        }
        break;
    default:
        return NULL;
    }

    /* Create stream */
    stream = &default_stream;
    stream->fd = fd;
    stream->flags = flags;

    /* Create write buffer if needed */
    if (flags & FILE_WRITE) {
        stream->write_buf = default_write_buf;
        stream->write_pos = stream->write_buf;
        stream->write_end = stream->write_buf + WRITE_BUF_SIZE;
    }

    return stream;
}

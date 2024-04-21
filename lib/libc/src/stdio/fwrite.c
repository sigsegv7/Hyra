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
#include <string.h>
#include <unistd.h>

size_t
fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    const char *pos;
    size_t n_remaining;

    if (ptr == NULL || stream == NULL)
        return 0;
    if (size == 0 || nmemb == 0)
        return 0;
    if (!(stream->flags & FILE_WRITE))
        return 0;

    /* Write data to buffer, flushing as needed */
    pos = ptr;
    n_remaining = size * nmemb;
    while (n_remaining > 0) {
        size_t n_free = stream->write_end - stream->write_pos;

        /* If enough buffer space is available, finish writing */
        if (n_remaining < n_free) {
            memcpy(stream->write_pos, pos, n_remaining);
            stream->write_pos += n_remaining;
            break;
        }

        /* Otherwise, write as much as possible now */
        memcpy(stream->write_pos, pos, n_free);
        stream->write_pos += n_free;

        /* Return if flushing fails */
        if (fflush(stream) == EOF)
            return nmemb - (n_remaining / size);

        pos += n_free;
        n_remaining -= n_free;
    }

    return nmemb;
}

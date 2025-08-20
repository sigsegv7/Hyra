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
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgfx/draw.h>
#include <libgfx/gfx.h>

static struct gfx_ctx ctx;

static ssize_t
rand_bytes(char *buf, size_t len)
{
    ssize_t retval;
    int fd;

    fd = open("/dev/random", O_RDONLY);
    if (fd < 0) {
        return fd;
    }

    if ((retval = read(fd, buf, len)) < 0) {
        close(fd);
        return retval;
    }

    close(fd);
    return retval;
}

static void
screensave(void)
{
    size_t n_iter = 0;          /* Monotonic */
    struct timespec ts;
    char randbuf[2];
    color_t curpix, nextpix;
    uint8_t step = 0;

    ts.tv_sec = 0;
    ts.tv_nsec = 3000000;

    /* Begin the radiation ::) */
    for (;;) {
        rand_bytes(randbuf, sizeof(randbuf));
        for (size_t i = 0; i < (ctx.fb_size / 4) - 1; i += step + 1) {
            curpix = ctx.io[i];
            nextpix = ctx.io[i + 1];

            /* If a multiple of 16, AND, otherwise XOR */
            if ((n_iter & 15) != 0) {
                curpix ^= randbuf[0] & 3;
                nextpix ^= (curpix | (nextpix << 1));
                nextpix ^= step;
            } else {
                curpix &= randbuf[0] & 3;
                nextpix &= (curpix | (nextpix << 1));
                nextpix &= step;
            }

            ctx.io[i] = curpix;
            ctx.io[i + 1] = nextpix;
        }

        sleep(&ts, &ts);
        if ((++step) > 50) {
            step = 0;
        }
        ++n_iter;
    }
}

int
main(void)
{
    int error;

    error = gfx_init(&ctx);
    if (error < 0) {
        printf("could not init libgfx\n");
        return error;
    }

    screensave();
    gfx_cleanup(&ctx);
    return 0;
}

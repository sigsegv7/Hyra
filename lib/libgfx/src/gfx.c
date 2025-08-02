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

#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/fbdev.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgfx/gfx.h>

/*
 * Initialize the libgfx context
 *
 * @res: Context pointer to be initialized
 */
int
gfx_init(struct gfx_ctx *res)
{
    struct fbattr attr;
    int fd, prot;

    if (res == NULL) {
        return -EINVAL;
    }

    /* Get framebuffer attributes */
    fd = open("/ctl/fb0/attr", O_RDONLY);
    if (fd < 0) {
        gfx_log("could not open '/ctl/fb0/attr");
        return fd;
    }

    read(fd, &attr, sizeof(attr));
    close(fd);
    res->fbdev = attr;

    /* Open the framebuffer file */
    res->fbfd = open("/dev/fb0", O_RDWR);
    if (res->fbfd < 0) {
        gfx_log("could not open '/dev/fb0'\n");
        return res->fbfd;
    }

    /* Map the framebuffer into memory */
    prot = PROT_READ | PROT_WRITE;
    res->fb_size = attr.height * attr.pitch;
    res->io = mmap(NULL, res->fb_size, prot, MAP_SHARED, res->fbfd, 0);

    /* Did the mmap() call work? */
    if (res->io == NULL) {
        gfx_log("could not map framebuffer\n");
        close(res->fbfd);
        return -1;
    }

    return 0;
}

/*
 * Cleanup all state and free the gfx
 * context.
 */
void
gfx_cleanup(struct gfx_ctx *ctx)
{
    if (ctx->io != NULL)
        munmap(ctx->io, ctx->fb_size);
    if (ctx->fbfd > 0)
        close(ctx->fbfd);
}

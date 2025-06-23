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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fbdev.h>
#include <kfg/window.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

static struct fbattr fbattr;
static uint32_t *framep;

static void
test_win(struct kfg_window *root, kfgpos_t x, kfgpos_t y, const char *str)
{
    struct kfg_text text;
    struct kfg_window *test_win;

    test_win = kfg_win_new(root, x, y);
    text.text = str;
    text.x = 0;
    text.y = 0;

    kfg_win_draw(root, test_win);
    kfg_win_putstr(test_win, &text);
}

int
main(void)
{
    int fb_fd, fbattr_fd, prot;
    size_t fb_size;
    struct kfg_window *root_win;

    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) {
        return fb_fd;
    }

    fbattr_fd = open("/ctl/fb0/attr", O_RDONLY);
    if (fbattr_fd < 0) {
        close(fb_fd);
        return fbattr_fd;
    }

    read(fbattr_fd, &fbattr, sizeof(fbattr));
    close(fbattr_fd);

    fb_size = fbattr.height * fbattr.pitch;
    prot = PROT_READ | PROT_WRITE;
    framep = mmap(NULL, fb_size, prot, MAP_SHARED, fb_fd, 0);

    root_win = malloc(sizeof(*root_win));
    root_win->x = 0;
    root_win->y = 0;
    root_win->width = fbattr.width;
    root_win->height = fbattr.height;
    root_win->fb_pitch = fbattr.pitch;
    root_win->framebuf = framep;
    root_win->bg = KFG_RED;
    root_win->border_bg = KFG_RED;
    test_win(root_win, 40, 85, "Hello, World!");
    test_win(root_win, 150, 20, "Mrow!");

    for (;;);
}

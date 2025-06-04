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
#include <unistd.h>

int
main(void)
{
    int fb_fd, fbattr_fd, prot;
    size_t fb_size;
    uint32_t *framep;
    struct fbattr fbattr;
    struct kfg_window root_win, test_win;

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

    root_win.x = 0;
    root_win.y = 0;
    root_win.width = fbattr.width;
    root_win.height = fbattr.height;
    root_win.fb_pitch = fbattr.pitch;
    root_win.framebuf = framep;
    root_win.bg = KFG_RED;
    root_win.border_bg = KFG_RED;

    test_win.x = 150;
    test_win.y = 85;
    test_win.width = 425;
    test_win.height = 350;
    test_win.fb_pitch = fbattr.pitch;
    test_win.framebuf = framep;
    test_win.bg = KFG_DARK;
    test_win.border_bg = KFG_RED;

    kfg_win_draw(&root_win, &test_win);
    for (;;);
}

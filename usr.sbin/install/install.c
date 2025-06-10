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
#include <sys/fbdev.h>
#include <sys/reboot.h>
#include <sys/spawn.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#define TEXT_STYLE "\033[37;44m"
#define INSTALLER_BG 0x00007F
#define BLOCK_SIZE 512
#define WAIT_KEY(VAR, C) while (((VAR = getchar())) != (C))

static struct fbattr fb_attr;
static void *fbmem;

struct progress_bar {
    bool dec;
    uint8_t progress;
};

/*
 * Clear the screen to a given background color
 *
 * @color: RGB value of chosen color
 * @setattr: Sets default text style if true
 */
static void
installer_clearscr(uint32_t color, bool setattr)
{
    size_t fb_size;
    uint32_t *p;

    if (setattr) {
        puts(TEXT_STYLE);
    }

    puts("\033[H");
    fb_size = fb_attr.height * fb_attr.pitch;
    p = fbmem;

    for (size_t i = 0; i < fb_size / sizeof(*p); ++i) {
        p[i] = color;
    }
}

static void
pre_installer(void)
{
    char c;

    puts("[S]hell/[I]nstall");
    for (;;) {
        c = getchar();
        if (c == 's') {
            puts("\033[0m");
            installer_clearscr(0x000000, false);
            spawn("/usr/bin/osh", SPAWN_WAIT);
            installer_clearscr(INSTALLER_BG, true);
            break;
        } else if (c == 'i') {
            break;
        }
    }
}

static void
reboot_prompt(void)
{
    char c;

    puts("Press 'r' to reboot");
    WAIT_KEY(c, 'r');
    cpu_reboot(REBOOT_RESET);
}

/*
 * Create a progress bar animation for long
 * operations.
 *
 * @bp: Pointer to a progress bar structure.
 * @n: Number of blocks operated on.
 * @max: Max blocks per bar update.
 */
static void
progress_update(struct progress_bar *bp, size_t n, size_t max)
{
    /*
     * We only want to update the progress bar
     * per `max' blocks written.
     */
    if ((n > 0) && (n % max) != 0) {
        return;
    }

    /* Add more '.' chars */
    if (bp->progress < 8 && !bp->dec) {
        fwrite(".", sizeof(char), 1, stdout);
    } else if (bp->progress >= 8) {
        bp->dec = true;
    }

    /* Remove '.' chars */
    if (bp->dec && bp->progress > 0) {
        fwrite("\b\f", sizeof(char), 2, stdout);
    } else if (bp->progress == 0) {
        bp->dec = false;
    }

    if (!bp->dec) {
        ++bp->progress;
    } else {
        --bp->progress;
    }
}

/*
 * Wipe a number of blocks beginning at the current
 * fd offset.
 *
 * @hdd_fd: Target drive fd
 * @count: Number of bytes to wipe
 */
static void
installer_wipe(int hdd_fd, uint32_t count)
{
    struct progress_bar bar = {0, 0};
    size_t write_blocks, nblocks;
    char buf[BLOCK_SIZE * 2];

    if (__unlikely(count == 0)) {
        puts("bad count for /dev/sd1");
        reboot_prompt();
    }

    count = ALIGN_UP(count, sizeof(buf));
    memset(buf, 0, sizeof(buf));
    write_blocks = sizeof(buf) / BLOCK_SIZE;
    nblocks = count / BLOCK_SIZE;

    /* Zero that shit */
    puts("zeroing...");
    for (int i = 0; i < nblocks; i += write_blocks) {
        write(hdd_fd, buf, sizeof(buf));
        progress_update(&bar, i, 256);
    }

    puts("OK");
}

static void
installer_run(void)
{
    struct stat hdd_sb, iso_sb;
    const char *hdd_path = "/dev/sd1";
    const char *iso_path = "/boot/Hyra.iso";
    char buf[256];
    int hdd_fd, error;
    char c;

    pre_installer();

    if ((hdd_fd = open(hdd_path, O_RDWR)) < 0) {
        puts("No available devices to target!");
        reboot_prompt();
    }

    snprintf(buf, sizeof(buf), "/dev/sd1 (%d sectors) [a]", hdd_sb.st_size);
    puts("Please choose which device to target");
    puts(buf);
    WAIT_KEY(c, 'a');

    /* Wait for y/n option */
    puts("\033[37;41m!! DRIVE WILL BE WIPED !!" TEXT_STYLE);
    puts("Are you sure? [y/n]");
    WAIT_KEY(c, 'y') {
        if (c == 'n') {
            reboot_prompt();
        }
    }

    puts("TODO");
    reboot_prompt();
}

int
main(void)
{
    int fb_fd, fbattr_fd, prot;
    size_t fb_size;

    if ((fb_fd = open("/dev/fb0", O_RDWR)) < 0) {
        puts("FATAL: failed to open /dev/fb0");
        return fb_fd;
    }
    if ((fbattr_fd = open("/ctl/fb0/attr", O_RDONLY)) < 0) {
        puts("FATAL: failed to open /ctl/fb0/attr");
        return fbattr_fd;
    }

    read(fbattr_fd, &fb_attr, sizeof(fb_attr));
    fb_size = fb_attr.height * fb_attr.pitch;

    prot = PROT_READ | PROT_WRITE;
    fbmem = mmap(NULL, fb_size, prot, MAP_SHARED, fb_fd, 0);

    installer_clearscr(INSTALLER_BG, true);
    puts("Welcome to Hyra/" _OSARCH " v" _OSVER "!");
    installer_run();
    return 0;
}

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
#include <sys/errno.h>
#include <sys/limine.h>
#include <sys/device.h>
#include <sys/driver.h>
#include <sys/fbdev.h>
#include <dev/video/fbdev.h>
#include <fs/devfs.h>
#include <fs/ctlfs.h>
#include <vm/vm.h>
#include <string.h>

#define FRAMEBUFFER \
        framebuffer_req.response->framebuffers[0]

static struct cdevsw fb_cdevsw;
static const struct ctlops fb_size_ctl;
static volatile struct limine_framebuffer_request framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static paddr_t
fbdev_mmap(dev_t dev, size_t size, off_t off, int flags)
{
    size_t max_bounds;

    max_bounds = FRAMEBUFFER->pitch * FRAMEBUFFER->height;
    if ((off + size) > max_bounds) {
        return 0;
    }

    return VIRT_TO_PHYS(FRAMEBUFFER->address);
}

static int
ctl_attr_read(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    struct fbattr attr;
    size_t len = sizeof(attr);

    if (sio == NULL) {
        return -EINVAL;
    }
    if (sio->buf == NULL) {
        return -EINVAL;
    }

    if (len > sio->len) {
        len = sio->len;
    }

    attr.width = FRAMEBUFFER->width;
    attr.height = FRAMEBUFFER->height;
    attr.pitch = FRAMEBUFFER->pitch;
    attr.bpp = FRAMEBUFFER->bpp;
    memcpy(sio->buf, &attr, len);
    return len;
}

struct fbdev
fbdev_get(void)
{
    struct fbdev ret;

    ret.mem = FRAMEBUFFER->address;
    ret.width = FRAMEBUFFER->width;
    ret.height = FRAMEBUFFER->height;
    ret.pitch = FRAMEBUFFER->pitch;
    ret.bpp = FRAMEBUFFER->bpp;
    return ret;
}

static int
fbdev_init(void)
{
    struct ctlfs_dev ctl;
    char devname[] = "fb0";
    devmajor_t major;
    dev_t dev;

    /* Register the device here */
    major = dev_alloc_major();
    dev = dev_alloc(major);
    dev_register(major, dev, &fb_cdevsw);
    devfs_create_entry(devname, major, dev, 0444);

    /* Register control files */
    ctl.mode = 0444;
    ctlfs_create_node(devname, &ctl);
    ctl.devname = devname;
    ctl.ops = &fb_size_ctl;
    ctlfs_create_entry("attr", &ctl);
    return 0;
}

static struct cdevsw fb_cdevsw = {
    .read = noread,
    .write = nowrite,
    .mmap = fbdev_mmap
};

static const struct ctlops fb_size_ctl = {
    .read = ctl_attr_read,
    .write = NULL,
};

DRIVER_EXPORT(fbdev_init, "fbdev");

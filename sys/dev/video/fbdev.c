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

#include <sys/types.h>
#include <sys/limine.h>
#include <sys/device.h>
#include <sys/driver.h>
#include <dev/video/fbdev.h>
#include <vm/vm.h>
#include <fs/devfs.h>

#define FRAMEBUFFER \
        framebuffer_req.response->framebuffers[0]

static volatile struct limine_framebuffer_request framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static struct device *dev;

static paddr_t
fbdev_mmap(struct device *dev, off_t off, vm_prot_t prot)
{
    struct fbdev fbdev = fbdev_get_front();
    paddr_t len = fbdev.width * fbdev.pitch;
    paddr_t base = VIRT_TO_PHYS(fbdev.mem);
    paddr_t max_paddr = base + len;

    if (base + off > max_paddr) {
        return 0;
    }

    return base + off;
}

struct fbdev
fbdev_get_front(void)
{
    struct fbdev ret;

    ret.mem = FRAMEBUFFER->address;
    ret.width = FRAMEBUFFER->width;
    ret.height = FRAMEBUFFER->height;
    ret.pitch = FRAMEBUFFER->pitch;
    return ret;
}

static int
fbdev_init(void)
{
    dev = DEVICE_ALLOC();
    dev->blocksize = 1;
    dev->read = NULL;
    dev->write = NULL;
    dev->mmap = fbdev_mmap;

    device_create(dev, device_alloc_major(), 1);
    devfs_add_dev("fb", dev);
    return 0;
}

DRIVER_EXPORT(fbdev_init);

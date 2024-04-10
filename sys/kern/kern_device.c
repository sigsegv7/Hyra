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

#include <sys/device.h>
#include <sys/queue.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/spinlock.h>

static TAILQ_HEAD(, device) devices;
static struct spinlock devices_lock = {0};
static bool device_list_init = false;

dev_t
device_alloc_major(void)
{
    static dev_t major = 1;
    return major++;
}

struct device *
device_fetch(dev_t major, dev_t minor)
{
    struct device *dev;

    TAILQ_FOREACH(dev, &devices, link) {
        if (dev->major == major && dev->minor == minor) {
            return dev;
        }
    }

    return NULL;
}

dev_t
device_create(struct device *dev, dev_t major, dev_t minor)
{
    if (dev == NULL || minor == 0) {
        return -EINVAL;
    }

    if (major == 0) {
        return -EINVAL;
    }

    if (!device_list_init) {
        TAILQ_INIT(&devices);
        device_list_init = true;
    }

    dev->major = major;
    dev->minor = minor;

    spinlock_acquire(&devices_lock);
    TAILQ_INSERT_HEAD(&devices, dev, link);
    spinlock_release(&devices_lock);
    return dev->major;
}

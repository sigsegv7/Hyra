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

#include <sys/device.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <vm/dynalloc.h>
#include <string.h>

#define MAX_MAJOR 256
#define MAX_MINOR 256

struct device_major {
    void **devsw_tab;
    uint16_t devsw_count;
};

static struct device_major devtab[MAX_MAJOR];

/*
 * Allocate a device major.
 *
 * Returns 0 on failure.
 */
devmajor_t
dev_alloc_major(void)
{
    static devmajor_t next = 1;

    if (next > MAX_MAJOR)
        return 0;

    return next++;
}

/*
 * Allocates a device minor.
 *
 * @major: Major number for class of device.
 *
 * Returns 0 on failure.
 */
dev_t
dev_alloc(devmajor_t major)
{
    struct device_major *devmajor;
    size_t allocsize;

    if (major >= MAX_MAJOR)
        return 0;

    devmajor = &devtab[major];
    if (devmajor->devsw_count >= MAX_MINOR)
        return 0;

    /*
     * Try to allocate a devsw table if needed.
     *
     * XXX: If MAX_MINOR is 256 then we'll need roughly 8 * 256
     *      (2048) bytes to store each table. Perhaps we should
     *      figure out a way we can save memory?
     */
    if (devmajor->devsw_tab == NULL) {
        allocsize = sizeof(void *) * MAX_MINOR;
        devmajor->devsw_tab = dynalloc(allocsize);

        if (devmajor->devsw_tab == NULL)
            return 0;

        memset(devmajor->devsw_tab, 0, allocsize);
    }

    return ++devmajor->devsw_count;
}

/*
 * Register a device.
 *
 * Returns less than zero value on failure.
 */
int
dev_register(devmajor_t major, dev_t dev, void *devsw)
{
    struct device_major *devmajor;

    if (major >= MAX_MAJOR)
        return -EINVAL;

    devmajor = &devtab[major];
    if (dev > devmajor->devsw_count)
        return -EINVAL;

    devmajor->devsw_tab[dev] = devsw;
    return 0;
}

/*
 * Get a devsw structure from device major
 * and minor numbers.
 *
 * @major: Major number to lookup.
 * @dev: Minor number to lookup.
 */
void *
dev_get(devmajor_t major, dev_t dev)
{
    struct device_major *devmajor;

    if (major >= MAX_MAJOR)
        return 0;

    devmajor = &devtab[major];
    if (dev > devmajor->devsw_count)
        return NULL;

    return devmajor->devsw_tab[dev];
}

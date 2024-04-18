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
#include <sys/errno.h>
#include <sys/sio.h>
#include <sys/vfs.h>
#include <sys/cdefs.h>
#include <fs/devfs.h>
#include <vm/obj.h>
#include <vm/vm.h>
#include <vm/pager.h>

static int dv_pager_get(struct vm_object *obj, off_t off, size_t len,
                        struct vm_page *pg);

static int dv_pager_store(struct vm_object *obj, off_t off, size_t len,
                          struct vm_page *pg);

static int dv_pager_paddr(struct vm_object *obj, paddr_t *paddr,
                          vm_prot_t prot);

/* Pager operations */
struct vm_pagerops g_dev_pagerops = {
    .get = dv_pager_get,
    .store = dv_pager_store,
    .get_paddr = dv_pager_paddr
};

/*
 * Fetch a device descriptor from a memory
 * object.
 *
 * Returns 0 on success.
 */
static int
dv_get_dev(struct vm_object *obj, struct device **res)
{
    struct device *dev;
    struct vnode *vp;
    int status;

    if ((vp = obj->vnode) == NULL)
        return -EIO;

    /* Now try to fetch the device */
    if ((status = devfs_get_dev(vp, &dev)) != 0)
        return status;

    *res = dev;
    return 0;
}

static int
dv_pager_paddr(struct vm_object *obj, paddr_t *paddr, vm_prot_t prot)
{
    struct device *dev;
    int status;

    if (paddr == NULL) {
        return -EINVAL;
    }

    /* Try to fetch the device */
    if ((status = dv_get_dev(obj, &dev)) != 0) {
        return status;
    }

    if (dev->mmap == NULL) {
        /*
         * mmap() is not supported, this is not an object
         * we can handle so try switching its operations
         * to the vnode pager ops...
         */
        obj->pgops = &g_vnode_pagerops;
        return -ENOTSUP;
    }

    *paddr = dev->mmap(dev, 0, prot);
    return 0;
}

static int
dv_pager_store(struct vm_object *obj, off_t off, size_t len, struct vm_page *pg)
{
    return -1;
}

static int
dv_pager_get(struct vm_object *obj, off_t off, size_t len, struct vm_page *pg)
{
    return -1;
}

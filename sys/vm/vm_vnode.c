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
#include <vm/obj.h>
#include <vm/vm.h>
#include <vm/pager.h>

static int vn_pager_get(struct vm_object *obj, off_t off, size_t len,
                        struct vm_page *pg);

static int vn_pager_store(struct vm_object *obj, off_t off, size_t len,
                          struct vm_page *pg);

/* Pager operations */
struct vm_pagerops g_vnode_pagerops = {
    .get = vn_pager_get,
    .store = vn_pager_store
};

static inline void
vn_prep_sio(struct sio_txn *sio, char *pg, off_t off, size_t len)
{
    sio->len = __ALIGN_DOWN(len, vm_get_page_size());
    sio->buf = pg;
    sio->offset = off;
    sio->len = len;
}

static int
vn_pager_io(struct vm_object *obj, off_t off, size_t len,
            struct vm_page *pg, bool out)
{
    struct sio_txn sio;
    struct vnode *vp;
    ssize_t paged_bytes;
    char *dest;
    int res = 0;

    if (obj == NULL || pg == NULL) {
        return -EIO;
    }

    spinlock_acquire(&obj->lock);
    dest = PHYS_TO_VIRT(pg->physaddr);

    /* Attempt to fetch the vnode */
    if ((vp = obj->vnode) == NULL) {
        res = -EIO;
        goto done;
    }

    /* Prepare the SIO transaction */
    vn_prep_sio(&sio, dest, off, len);
    if (sio.len == 0) {
        res = -EIO;
        goto done;
    }

    /* Perform I/O on the backing store */
    paged_bytes = out ? vfs_write(vp, &sio) : vfs_read(vp, &sio);
    if (paged_bytes < 0) {
        /* Failure */
        res = paged_bytes;
        goto done;
    }
done:
    spinlock_release(&obj->lock);
    return res;
}

static int
vn_pager_store(struct vm_object *obj, off_t off, size_t len, struct vm_page *pg)
{
    return vn_pager_io(obj, off, len, pg, true);
}

static int
vn_pager_get(struct vm_object *obj, off_t off, size_t len, struct vm_page *pg)
{
    return vn_pager_io(obj, off, len, pg, false);
}

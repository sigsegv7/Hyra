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
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <vm/vm_vnode.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>
#include <vm/vm.h>

#define VN_TIMEOUT_USEC 200000

#define pr_trace(fmt, ...) kprintf("vm_vnode: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

/* Debug print helper */
#if defined(PR_DEBUG)
#define pr_debug(...) pr_trace(__VA_ARGS__)
#else
#define pr_debug(...) __nothing
#endif  /* PR_DEBUG */

const struct vm_pagerops vm_vnops;

/*
 * Perform read/write operation on vnode to/from pages.
 *
 * Returns number of bytes read.
 */
static int
vn_io(struct vnode *vp, struct vm_page **pgs, unsigned int npages, int rw)
{
    struct vop_getattr_args args;
    struct sio_txn sio;
    struct vattr vattr;
    size_t c, read = 0;
    int err;

    /* TODO: Add support for writes */
    if (rw != 0) {
        return -ENOTSUP;
    }

    args.vp = vp;
    args.res = &vattr;
    c = MAX(vattr.size / DEFAULT_PAGESIZE, 1);

    if ((err = vfs_vop_getattr(vp, &args)) != 0) {
        return err;
    }

    if (npages > c) {
        npages = c;
    }

    /* Prepare SIO constants */
    sio.len = DEFAULT_PAGESIZE;
    sio.offset = 0;

    /* Copy in each page */
    for (size_t i = 0; i < npages; ++i) {
        sio.buf = PHYS_TO_VIRT(pgs[i]->phys_addr);
        read = vfs_vop_read(vp, &sio);
        if (read < 0)
            pr_debug("vn_io: page-in @ %p failed (err=%d)\n", err);
    }

    return read;
}

/*
 * Get pages from backing store.
 *
 * @obp: Object representing the backing store.
 * @pgs: Page descriptors to be filled.
 * @off: Offset to read from in backing store.
 * @len: Length to read in bytes.
 */
static int
vn_get(struct vm_object *obp, struct vm_page **pgs, off_t off, size_t len)
{
    struct vm_page *pgtmp;
    size_t j, pgcnt;

    spinlock_acquire(&obp->lock);

    for (int i = off; i < len; i += DEFAULT_PAGESIZE) {
        j = i / DEFAULT_PAGESIZE;
        pgtmp = vm_pagelookup(obp, i);

        /*
         * If we have no corresponding page in the object
         * at this offset, we will need to make our own.
         */
        if (pgtmp == NULL) {
            pgtmp = vm_pagealloc(obp, PALLOC_ZERO);
            if (pgtmp == NULL) {
                pgs[j]->flags &= ~PG_VALID;
                continue;
            }
            pgtmp->offset = i;
            pgs[j] = pgtmp;
        }

        if (spinlock_usleep(&pgtmp->lock, VN_TIMEOUT_USEC) != 0) {
            pgs[j]->flags &= ~PG_VALID;
            continue;
        }

        /* Set the page count and ensure it is valid */
        pgcnt = ALIGN_DOWN(len, DEFAULT_PAGESIZE);
        pgcnt = MAX(pgcnt, 1);

        /* Page in and save this pgtmp */
        vn_io(obp->data, &pgtmp, pgcnt, 0);
        pgs[j] = pgtmp;
        spinlock_release(&pgtmp->lock);
    }

    spinlock_release(&obp->lock);
    return 0;
}

/*
 * Attach a virtual memory object to a vnode.
 *
 * @vp: Vnode to attach to.
 */
struct vm_object *
vn_attach(struct vnode *vp, vm_prot_t prot)
{
    struct vm_object *vmobj;
    int error;

    if (vp->type != VREG) {
        pr_error("vn_attach: vp=%p, prot=%x\n", vp, prot);
        pr_error("vn_attach: Special files not supported yet!\n");
        return NULL;
    }

    error = vm_obj_init(&vp->vobj, &vm_vnops, 1);
    if (error != 0) {
        return NULL;
    }

    vmobj = &vp->vobj;
    vmobj->prot = prot;
    vmobj->data = vp;
    vmobj->pgops = &vm_vnops;
    return vmobj;
}

const struct vm_pagerops vm_vnops = {
    .get = vn_get
};

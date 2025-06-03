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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/syslog.h>
#include <sys/mman.h>
#include <sys/filedesc.h>
#include <vm/dynalloc.h>
#include <vm/vm_pager.h>
#include <vm/vm_device.h>
#include <vm/pmap.h>
#include <vm/map.h>
#include <vm/vm.h>
#include <assert.h>

#define pr_trace(fmt, ...) kprintf("vm_map: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

RBT_GENERATE(lgdr_entries, mmap_entry, hd, mmap_entrycmp);

static inline void
mmap_dbg(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    pr_trace("addr=%p, len=%d, prot=%x\nflags=%x, fildes=%d, off=%d\n",
        addr, len, prot, flags, fildes, off);
}

/*
 * Add a memory mapping to the mmap ledger.
 *
 * @td: Process to add mapping to.
 * @ep: Memory map entry to add.
 * @len: Length of memory mapping in bytes.
 */
static inline int
mmap_add(struct proc *td, struct mmap_entry *ep)
{
    struct mmap_entry *tmp;
    struct mmap_lgdr *lp = td->mlgdr;

    if (ep->size == 0) {
        return -EINVAL;
    }

    tmp = RBT_INSERT(lgdr_entries, &lp->hd, ep);
    __assert(tmp == NULL);
    lp->nbytes += ep->size;
    return 0;
}

/*
 * Remove memory mapping from mmap ledger
 *
 * @td: Process to remove mapping from.
 * @ep: Memory map entry to remove.
 */
static inline void
mmap_remove(struct proc *td, struct mmap_entry *ep)
{
    struct mmap_lgdr *lp = td->mlgdr;

    RBT_REMOVE(lgdr_entries, &lp->hd, ep);
    lp->nbytes -= ep->size;
    dynfree(ep);
}

/*
 * Create/destroy virtual memory mappings in a specific
 * address space.
 *
 * @vas: Address space.
 * @va: Virtual address.
 * @pa: Physical address (zero if `unmap` is true)
 * @prot: Protection flags (zero if `unmap` is true)
 * @unmap: True to unmap.
 * @count: Count of bytes to be mapped which is aligned to the
 *         machine's page granularity.
 *
 * Returns 0 on success. On failure, this routine
 * returns a less than zero value or greater than zero
 * value that contains the offset into the memory where the
 * mapping failed.
 */
static int
vm_map_modify(struct vas vas, vaddr_t va, paddr_t pa, vm_prot_t prot, bool unmap,
              size_t count)
{
    size_t misalign = va & (DEFAULT_PAGESIZE - 1);
    int s;

    if (count == 0) {
        return -EINVAL;
    }

    /* Ensure we fully span pages */
    count = ALIGN_UP(count + misalign, DEFAULT_PAGESIZE);
    va = ALIGN_DOWN(va, DEFAULT_PAGESIZE);
    pa = ALIGN_DOWN(pa, DEFAULT_PAGESIZE);

    for (uintptr_t i = 0; i < count; i += DEFAULT_PAGESIZE) {
        /* See if we should map or unmap the range */
        if (unmap) {
            s = pmap_unmap(vas, va + i);
        } else {
            s = pmap_map(vas, va + i, pa + i, prot);
        }

        if (s != 0) {
            /* Something went wrong, return the offset */
            return i;
        }
    }

    return 0;
}

/*
 * Create a physical to virtual memory mapping.
 *
 * @addr: Virtual address to map (NULL to be any).
 * @len: The amount of bytes to map (must be page aligned)
 * @prot: Protection flags (PROT_*)
 * @fildes: File descriptor.
 * @off: Offset.
 *
 * TODO: Fields to use: `fildes' and `off'
 * XXX: Must be called after pid 1 is up and running to avoid
 *      crashes.
 */
void *
mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    struct vm_object *map_obj = NULL;
    struct cdevsw *cdevp;
    struct vm_page *pg;
    struct mmap_entry *ep;
    struct vnode *vp;
    struct filedesc *fdp;
    struct proc *td;
    struct vas vas;
    int error, npgs;
    paddr_t pa;
    vaddr_t va;
    size_t misalign;

    misalign = len & (DEFAULT_PAGESIZE - 1);
    len = ALIGN_UP(len + misalign, DEFAULT_PAGESIZE);
    npgs = len / DEFAULT_PAGESIZE;
    vas = pmap_read_vas();

    if (addr == NULL) {
        pr_error("mmap: NULL addr not supported\n");
        return NULL;
    }

    /* Validate flags */
    if (ISSET(flags, MAP_FIXED)) {
        pr_error("mmap: fixed mappings not yet supported\n");
        mmap_dbg(addr, len, prot, flags, fildes, off);
        return NULL;
    }

    /*
     * Attempt to open the file if mapping
     * is shared.
     */
    if (ISSET(flags, MAP_SHARED)) {
        fdp = fd_get(fildes);
        if (fdp == NULL) {
            pr_error("mmap: no such fd (fd=%d)\n", fildes);
            return NULL;
        }

        vp = fdp->vp;
        if (vp->type != VCHR) {
            /* TODO */
            pr_error("mmap: only device files supported\n");
            return NULL;
        }

        map_obj = dv_attach(vp->major, vp->dev, prot);
        if (map_obj == NULL) {
            kprintf("mmap: dv_attach() failure\n");
            return NULL;
        }

        cdevp = map_obj->data;
        if ((pa = cdevp->mmap(vp->dev, len, off, 0)) == 0) {
            kprintf("mmap: dev mmap() gave 0\n");
            return NULL;
        }

        va = ALIGN_DOWN((vaddr_t)addr, DEFAULT_PAGESIZE);
        error = vm_map(vas, va, pa, prot, len);
        if (error != 0) {
            kprintf("mmap: map failed (error=%d)\n", error);
            return NULL;
        }

        goto done;
    }

    /* Only allocate new obj if needed */
    if (map_obj == NULL) {
        map_obj = dynalloc(sizeof(*map_obj));
        if (map_obj == NULL) {
            kprintf("mmap: failed to allocate map object\n");
            return NULL;
        }
        error = vm_obj_init(map_obj, &vm_anonops, 1);
        if (error < 0) {
            kprintf("mmap: vm_obj_init() returned %d\n", error);
            kprintf("mmap: failed to init object\n");
            return NULL;
        }
    }

    /* XXX: Assuming private */
    va = ALIGN_DOWN((vaddr_t)addr, DEFAULT_PAGESIZE);

    for (int i = 0; i < npgs; ++i) {
        pg = vm_pagealloc(map_obj, PALLOC_ZERO);

        if (pg == NULL) {
            /* TODO */
            pr_error("mmap: failed to allocate page %d\n");
            return NULL;
        }

        pa = pg->phys_addr;
        error = vm_map(vas, va, pa, prot, len);
        if (error < 0) {
            pr_error("mmap: failed to map page (retval=%x)\n", error);
            return NULL;
        }
    }

done:
    /* Add entry to ledger */
    td = this_td();
    ep = dynalloc(sizeof(*ep));
    if (ep == NULL) {
        pr_error("mmap: failed to allocate mmap ledger entry\n");
        return NULL;
    }

    ep->va_start = va;
    ep->obj = map_obj;
    ep->size = len;
    mmap_add(td, ep);
    return addr;
}

/*
 * Remove mappings for entire pages that
 * belong to the current process.
 *
 * XXX: POSIX munmap(3) requires `addr' to be page-aligned
 *      and will return -EINVAL if otherwise. However, with
 *      OUSI munmap(3), `addr' is rounded down to the nearest
 *      multiple of the machine page size.
 */
int
munmap(void *addr, size_t len)
{
    int pgno;
    vaddr_t va;
    struct proc *td;
    struct mmap_lgdr *lp;
    struct mmap_entry find, *res;
    struct vas vas;

    if (addr == NULL || len == 0) {
        return -EINVAL;
    }

    /* Apply machine specific addr/len adjustments */
    va = ALIGN_DOWN((vaddr_t)addr, DEFAULT_PAGESIZE);
    len = ALIGN_UP(len, DEFAULT_PAGESIZE);
    pgno = va >> 12;

    td = this_td();
    __assert(td != NULL && "no pid 1");
    vas = pmap_read_vas();

    /*
     * Try to get the mmap ledger, should not run into
     * any issues as long as the PCB isn't borked. However,
     * if it somehow is, just segfault ourselves.
     */
    if ((lp = td->mlgdr) == NULL) {
        __sigraise(SIGSEGV);
        return -EFAULT;     /* Unreachable */
    }

    /* Lookup entry in ledger with virtual address */
    find.va_start = va;
    res = RBT_FIND(lgdr_entries, &lp->hd, &find);
    if (res == NULL) {
        pr_error("munmap: page %d not in ledger\n", pgno);
        return -EINVAL;
    }

    vm_unmap(vas, va, len);
    mmap_remove(td, res);
    return 0;
}

/*
 * mmap() syscall
 *
 * arg0 -> addr
 * arg1 -> len
 * arg2 -> prot
 * arg3 -> flags
 * arg4 -> fildes
 * arg5 -> off
 */
scret_t
sys_mmap(struct syscall_args *scargs)
{
    void *addr;
    size_t len;
    int prot, flags;
    int fildes, off;

    addr = (void *)scargs->arg0;
    len = scargs->arg1;
    prot = scargs->arg2 | PROT_USER;
    flags = scargs->arg3;
    fildes = scargs->arg4;
    off = scargs->arg5;
    return (scret_t)mmap(addr, len, prot, flags, fildes, off);
}

/*
 * munmap() syscall
 *
 * arg0 -> addr
 * arg1 -> len
 */
scret_t
sys_munmap(struct syscall_args *scargs)
{
    void *addr;
    size_t len;

    addr = (void *)scargs->arg0;
    len = scargs->arg1;
    return (scret_t)munmap(addr, len);
}

/*
 * Create a virtual memory mapping in a specific
 * address space.
 *
 * @vas: Address space.
 * @va: Virtual address.
 * @pa: Physical address.
 * @prot: Protection flags.
 * @count: Count of bytes to be mapped (aligned to page granularity).
 *
 * Returns 0 on success, and a less than zero errno
 * on failure.
 */
int
vm_map(struct vas vas, vaddr_t va, paddr_t pa, vm_prot_t prot, size_t count)
{
    int retval;

    va = ALIGN_UP(va, DEFAULT_PAGESIZE);
    retval = vm_map_modify(vas, va, pa, prot, false, count);

    /* Return on success */
    if (retval == 0)
        return 0;

    /* Failure, clean up */
    for (int i = 0; i < retval; i += DEFAULT_PAGESIZE) {
        pmap_unmap(vas, va + i);
    }

    return -1;
}

/*
 * Unmap a virtual memory mapping in a specific
 * address space.
 *
 * @vas: Address space.
 * @va: Virtual address to start unmapping at.
 * @count: Bytes to unmap (aligned to page granularity)
 */
int
vm_unmap(struct vas vas, vaddr_t va, size_t count)
{
    return vm_map_modify(vas, va, 0, 0, true, count);
}

/*
 * Helper for tree(3) and the mmap ledger.
 */
int
mmap_entrycmp(const struct mmap_entry *a, const struct mmap_entry *b)
{
    return (a->va_start < b->va_start) ? -1 : a->va_start > b->va_start;
}

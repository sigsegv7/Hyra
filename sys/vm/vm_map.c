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

#include <vm/map.h>
#include <vm/vm.h>
#include <vm/pmap.h>
#include <vm/physseg.h>
#include <vm/dynalloc.h>
#include <vm/pager.h>
#include <sys/types.h>
#include <sys/filedesc.h>
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/sched.h>
#include <lib/assert.h>

#define ALLOC_MAPPING() dynalloc(sizeof(struct vm_mapping))
#define DESTROY_MAPPING(MAPPING) dynfree(MAPPING)

static size_t
vm_hash_vaddr(vaddr_t va) {
    va = (va ^ (va >> 30)) * (size_t)0xBF58476D1CE4E5B9;
    va = (va ^ (va >> 27)) * (size_t)0x94D049BB133111EB;
    va = va ^ (va >> 31);
    return va;
}

/*
 * Destroy a map queue.
 */
void
vm_free_mapq(vm_mapq_t *mapq)
{
    struct vm_mapping *map;
    size_t map_pages, granule;

    granule = vm_get_page_size();
    TAILQ_FOREACH(map, mapq, link) {
        map_pages = (map->range.end - map->range.start) / granule;
        vm_free_pageframe(map->range.start, map_pages);
    }
    dynfree(map);
}

/*
 * Remove a mapping from a mapspace.
 *
 * @ms: Mapspace.
 * @mapping: Mapping to remove.
 */
void
vm_mapspace_remove(struct vm_mapspace *ms, struct vm_mapping *mapping)
{
    size_t vhash;
    vm_mapq_t *mapq;

    if (ms == NULL)
        return;

    vhash = vm_hash_vaddr(mapping->range.start);
    mapq = &ms->mtab[vhash % MTAB_ENTRIES];
    TAILQ_REMOVE(mapq, mapping, link);
    --ms->map_count;
}

/*
 * Fetch a mapping from a mapspace.
 *
 * @ms: Mapspace.
 * @va: Virtual address.
 */
struct vm_mapping *
vm_mapping_fetch(struct vm_mapspace *ms, vaddr_t va)
{
    size_t vhash;
    const vm_mapq_t *mapq;
    struct vm_mapping *map;

    if (ms == NULL)
        return NULL;

    vhash = vm_hash_vaddr(va);
    mapq = &ms->mtab[vhash % MTAB_ENTRIES];

    TAILQ_FOREACH(map, mapq, link) {
        if (map->vhash == vhash) {
            return map;
        }
    }

    return NULL;
}

/*
 * Insert a mapping into a mapspace.
 *
 * @ms: Target mapspace.
 * @mapping: Mapping to insert.
 */
void
vm_mapspace_insert(struct vm_mapspace *ms, struct vm_mapping *mapping)
{
    size_t vhash;
    vm_mapq_t *q;

    if (mapping == NULL || ms == NULL)
        return;

    vhash = vm_hash_vaddr(mapping->range.start);
    mapping->vhash = vhash;

    q = &ms->mtab[vhash % MTAB_ENTRIES];
    TAILQ_INSERT_HEAD(q, mapping, link);
    ++ms->map_count;
}

/*
 * Create a mapping (internal helper)
 *
 * @addr: Address to map.
 * @physmem: Physical address, set to 0 to alloc one here
 * @prot: Protection flags.
 *
 * Returns zero on failure.
 */
static paddr_t
vm_map(void *addr, paddr_t physmem, vm_prot_t prot, size_t len)
{
    struct proc *td = this_td();
    const size_t GRANULE = vm_get_page_size();

    int status;

    /* Allocate the physical memory if needed */
    if (physmem == 0)
        physmem = vm_alloc_pageframe(len / GRANULE);

    if (physmem == 0)
        return 0;

    /*
     * XXX: There is no need to worry about alignment yet
     *      as vm_map_create() handles that internally.
     */
    prot |= PROT_USER;
    status = vm_map_create(td->addrsp, (vaddr_t)addr, physmem, prot, len);
    if (status != 0) {
        vm_free_pageframe(physmem, len / GRANULE);
        return 0;
    }

    return physmem;
}

/*
 * Create a mapping backed by a file.
 *
 * @addr: Address to map.
 * @prot: Protection flags.
 * @len: Length of mapping.
 * @off: Offset.
 * @fd: File descriptor.
 */
static paddr_t
vm_fd_map(void *addr, vm_prot_t prot, size_t len, off_t off, int fd,
          struct vm_mapping *mapping)
{
    paddr_t physmem = 0;

    int oflag;
    struct filedesc *filedes;
    struct vnode *vp;

    struct proc *td = this_td();
    struct vm_page pg = {0};

    /* Attempt to get the vnode */
    filedes = fd_from_fdnum(td, fd);
    if (filedes == NULL)
        return 0;
    if ((vp = filedes->vnode) == NULL)
        return 0;

    /* Check the perms of the filedes */
    oflag = filedes->oflag;
    if (__TEST(prot, PROT_WRITE) && oflag == O_RDONLY)
        return 0;
    if (!__TEST(prot, PROT_WRITE) && oflag == O_WRONLY)
        return 0;

    /* Try to create the virtual memory object */
    if (vm_obj_init(&vp->vmobj, vp) != 0)
        return 0;

    mapping->vmobj = vp->vmobj;
    vm_object_ref(vp->vmobj);

    /* Try to fetch a physical address */
    if (vm_pager_paddr(vp->vmobj, &physmem, prot) != 0) {
        vm_obj_destroy(vp->vmobj);
        return 0;
    }

    /*
     * If the pager found a physical address for the object to
     * be mapped to, then we start off with an anonymous mapping
     * then connect it to the physical address (creates a shared mapping)
     */
    if (physmem != 0) {
        vm_map(addr, physmem, prot, len);
        return physmem;
    }

    /*
     * If the pager could not find a physical address for
     * the object to be mapped to, start of with just a plain
     * anonymous mapping then page-in from whatever filesystem
     * (creates a shared mapping)
     */
    physmem = vm_map(addr, 0, prot, len);
    pg.physaddr = physmem;

    if (vm_pager_get(vp->vmobj, off, len, &pg) != 0) {
        vm_obj_destroy(vp->vmobj);
        return 0;
    }

    return physmem;
}

static int
munmap(void *addr, size_t len)
{
    struct proc *td = this_td();
    struct vm_mapping *mapping;

    struct vm_object *obj;
    struct vnode *vp;

    struct vm_mapspace *ms;
    size_t map_len, granule;
    vaddr_t map_start, map_end;

    spinlock_acquire(&td->mapspace_lock);
    ms = &td->mapspace;

    granule = vm_get_page_size();
    mapping = vm_mapping_fetch(ms, (vaddr_t)addr);
    if (mapping == NULL) {
        return -1;
    }

    map_start = mapping->range.start;
    map_end = mapping->range.end;
    map_len = map_end - map_start;

    /* Try to release any virtual memory objects */
    if ((obj = mapping->vmobj) != NULL) {
        spinlock_acquire(&obj->lock);
        /*
         * Drop our ref and try to cleanup. If the refcount
         * is > 1, something is still holding it and we can't
         * do much.
         */
        vm_object_unref(obj);
        vp = obj->vnode;
        if (vp != NULL && obj->ref == 1) {
            vp->vmobj = NULL;
            vm_obj_destroy(obj);
        }
        spinlock_release(&obj->lock);
    }

    /* Release the mapping */
    vm_map_destroy(td->addrsp, map_start, map_len);
    vm_free_pageframe(mapping->range.start, map_len / granule);

    /* Destroy the mapping descriptor */
    vm_mapspace_remove(ms, mapping);
    dynfree(mapping);
    spinlock_release(&td->mapspace_lock);
    return 0;
}

static void *
mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    const int PROT_MASK = PROT_WRITE | PROT_EXEC;
    const size_t GRANULE = vm_get_page_size();
    uintptr_t map_end, map_start;

    struct proc *td = this_td();
    struct vm_mapping *mapping;
    struct vm_object *vmobj;

    size_t misalign = ((vaddr_t)addr) & (GRANULE - 1);
    paddr_t physmem = 0;

    /* Ensure of valid prot flags */
    if ((prot & ~PROT_MASK) != 0)
        return MAP_FAILED;

    /* Try to allocate a mapping */
    mapping = ALLOC_MAPPING();
    if (mapping == NULL)
        return MAP_FAILED;

    mapping->prot = prot | PROT_USER;

    /*
     * Now we check what type of map request
     * this is.
     */
    if (__TEST(flags, MAP_ANONYMOUS)) {
        /* Try to create a virtual memory object */
        if (vm_obj_init(&vmobj, NULL) != 0)
            goto fail;

        /*
         * If 'addr' is NULL, we'll just allocate physical
         * memory right away.
         */
        if (addr == NULL)
            physmem = vm_alloc_pageframe(len / GRANULE);

        /*
         * Enable demand paging for this object if
         * `addr` is not NULL.
         */
        if (addr != NULL) {
            vmobj->is_anon = 1;
            vmobj->demand = 1;

            mapping->vmobj = vmobj;
            mapping->physmem_base = 0;
        } else if (physmem != 0) {
            vm_map((void *)physmem, physmem, prot, len);
            addr = (void *)physmem;

            vmobj->is_anon = 1;
            vmobj->demand = 0;
            mapping->vmobj = vmobj;
            mapping->physmem_base = physmem;
        }

        /* Did this work? */
        if (physmem == 0 && addr == NULL) {
            goto fail;
        }
    } else if (__TEST(flags, MAP_SHARED)) {
        physmem = vm_fd_map(addr, prot, len, off, fildes, mapping);
        if (physmem == 0) {
            goto fail;
        }
    }

    map_start = __ALIGN_DOWN((vaddr_t)addr, GRANULE);
    map_end = map_start + __ALIGN_UP(len + misalign, GRANULE);

    mapping->range.start = map_start;
    mapping->range.end = map_end;
    mapping->physmem_base = physmem;

    /* Add to mapspace */
    spinlock_acquire(&td->mapspace_lock);
    vm_mapspace_insert(&td->mapspace, mapping);
    spinlock_release(&td->mapspace_lock);
    return (void *)addr;
fail:
    DESTROY_MAPPING(mapping);
    return MAP_FAILED;
}

/*
 * Internal routine for cleaning up.
 *
 * @va: VA to start unmapping at.
 * @bytes_aligned: Amount of bytes to unmap.
 *
 * XXX DANGER!!: `bytes_aligned' is expected to be aligned by the
 *               machine's page granule. If this is not so,
 *               undefined behaviour will occur. This will
 *               be enforced via a panic.
 */
static void
vm_map_cleanup(struct vas vas, struct vm_ctx *ctx, vaddr_t va,
               size_t bytes_aligned, size_t granule)
{
    __assert(bytes_aligned != 0);
    __assert((bytes_aligned & (granule - 1)) == 0);

    for (size_t i = 0; i < bytes_aligned; i += 0x1000) {
        if (pmap_unmap(ctx, vas, va + i) != 0) {
            /*
             * XXX: This shouldn't happen... If it somehow does,
             *      then this should be handled.
             */
            panic("Could not cleanup!!!\n");
        }
    }
}

/*
 * Create a virtual memory mappings in the current
 * address space.
 *
 * @va: Virtual address.
 * @pa: Physical address.
 * @prot: Protection flags.
 * @bytes: Amount of bytes to be mapped. This is aligned by the
 *         machine's page granule, typically a 4k boundary.
 */
int
vm_map_create(struct vas vas, vaddr_t va, paddr_t pa, vm_prot_t prot, size_t bytes)
{
    size_t granule = vm_get_page_size();
    size_t misalign = va & (granule - 1);
    int s;

    struct vm_ctx *ctx = vm_get_ctx();

    /*
     * The amount of bytes to be mapped should fully span pages,
     * so we ensure it is aligned by the page granularity.
     */
    bytes = __ALIGN_UP(bytes + misalign, granule);

    /* Align VA/PA by granule */
    va = __ALIGN_DOWN(va, granule);
    pa = __ALIGN_DOWN(pa, granule);

    if (bytes == 0) {
        /* You can't map 0 pages, silly! */
        return -1;
    }

    for (uintptr_t i = 0; i < bytes; i += granule) {
        s = pmap_map(ctx, vas, va + i, pa + i, prot);
        if (s != 0) {
            /* Something went a bit wrong here, cleanup */
            vm_map_cleanup(vas, ctx, va, i, bytes);
            return -1;
        }
    }

    return 0;
}

/*
 * Destroy a virtual memory mapping in the current
 * address space.
 */
int
vm_map_destroy(struct vas vas, vaddr_t va, size_t bytes)
{
    struct vm_ctx *ctx = vm_get_ctx();
    size_t granule = vm_get_page_size();
    size_t misalign = va & (granule - 1);
    int s;

    /* We want bytes to be aligned by the granule */
    bytes = __ALIGN_UP(bytes + misalign, granule);

    /* Align VA by granule */
    va = __ALIGN_DOWN(va, granule);

    if (bytes == 0) {
        return -1;
    }

    for (uintptr_t i = 0; i < bytes; i += granule) {
        s = pmap_unmap(ctx, vas, va + i);
        if (s != 0) {
            return -1;
        }
    }

    return 0;
}

uint64_t
sys_mmap(struct syscall_args *args)
{
    return (uintptr_t)mmap((void *)args->arg0, args->arg1, args->arg2,
                           args->arg3, args->arg4, args->arg5);
}

uint64_t
sys_munmap(struct syscall_args *args)
{
    return munmap((void *)args->arg0, args->arg1);
}

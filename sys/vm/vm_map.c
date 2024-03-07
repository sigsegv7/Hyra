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
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <lib/assert.h>

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
    int s;

    struct vm_ctx *ctx = vm_get_ctx();

    /* We want bytes to be aligned by the granule */
    bytes = __ALIGN_UP(bytes, granule);

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
    int s;

    /* We want bytes to be aligned by the granule */
    bytes = __ALIGN_UP(bytes, granule);

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

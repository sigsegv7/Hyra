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

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <machine/bus.h>
#include <vm/physseg.h>
#include <vm/pmap.h>
#include <vm/map.h>
#include <vm/vm.h>

/*
 * Map a physical device address into the kernel address
 * space.
 *
 * @addr: Physical address to map.
 * @size: Size to map.
 * @flags: Mapping flags.
 * @vap: Resulting virtual address.
 */
int
bus_map(bus_addr_t addr, size_t size, int flags, void **vap)
{
    struct vm_ctx *vmctx = vm_get_ctx();
    struct vas vas;
    size_t pagesize;
    vaddr_t va;
    int status;

    pagesize = vm_get_page_size();
    size = __ALIGN_UP(size, pagesize);

    va = (vaddr_t)PHYS_TO_VIRT(addr);
    vas = pmap_read_vas();

    /* Ensure the size is valid */
    if (size == 0) {
        return -EINVAL;
    }

    /* Map the device address to our new VA */
    if ((status = vm_map_create(vas, va, addr, PROT_WRITE, size)) != 0) {
        return status;
    }

    /*
     * Mark the memory as uncachable because this is
     * for device I/O
     */
    if ((status = pmap_set_cache(vmctx, vas, va, VM_CACHE_UC)) != 0) {
        return status;
    }

    *vap = (void *)va;
    return 0;
}

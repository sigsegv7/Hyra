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
#include <vm/pmap.h>
#include <vm/map.h>
#include <vm/vm.h>

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
 * Create a virtual memory mapping in a specific
 * address space.
 *
 * @vas: Address space.
 * @va: Virtual address.
 * @pa: Physical address.
 * @prot: Protection flags.
 * @count: Count of bytes to be mapped (aligned to page granularity).
 *
 *  Returns 0 on success, and a less than zero errno
 *  on failure.
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

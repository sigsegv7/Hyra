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
#include <machine/bus.h>
#include <vm/vm.h>
#include <vm/map.h>
#include <vm/pmap.h>
#include <vm/dynalloc.h>
#include <dev/pci/resource.h>
#include <string.h>

#define BUS_PCI "PCI"
#define BUS_LPC "LPC"

/*
 * Set bus specific fields
 *
 * @brp: Bus resource pointer.
 * @type: Bus name/signature
 *
 * XXX: Assumes brp->base has already been set to
 *      a valid bus address.
 */
static int
bus_set(struct bus_resource *brp, const char *type)
{
    struct bus_op *io = &brp->io;

    switch (*type) {
    case 'P':
        if (strcmp(type, BUS_PCI) == 0) {
            /*
             * Bus addresses for 64-bit PCI can theoretically
             * span 2^64 bytes (i.e., the full 64-bit address space)
             */
            brp->dma_max = (bus_addr_t)-1;
            brp->dma_min = 0;

            /* Set hooks */
            io->enable_dma = pcir_enable_dma;
            io->disable_dma = pcir_disable_dma;
            io->set_sem = pcir_set_sem;
            io->clr_sem = pcir_clr_sem;
            io->dma_alloc = pcir_dma_alloc;
            io->dma_free = pcir_dma_free;
            io->dma_in = pcir_dma_in;
            io->dma_out = pcir_dma_out;
            return 0;
        }
    }

    return -ENODEV;
}

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
    struct vas vas;
    int status;
    vaddr_t va;
    vm_prot_t prot = PROT_READ | PROT_WRITE;

    /* Make sure we have a valid size */
    if (size == 0) {
        return -EINVAL;
    }

    size = ALIGN_UP(size, DEFAULT_PAGESIZE);
    va = addr + MMIO_OFFSET;
    vas = pmap_read_vas();

    /* Now map it to the higher half */
    if ((status = vm_map(vas, va, addr, prot, size)) != 0) {
        return status;
    }

    /*
     * Mark the memory as uncachable as this is for device I/O
     * and we do not want to get stale data.
     */
    if ((status = pmap_set_cache(vas, va, VM_CACHE_UC)) != 0) {
        vm_unmap(vas, va, size);
        return status;
    }

    *vap = (void *)va;
    return 0;
}

/*
 * Associate a specific bus with a 'bus_resource`
 * structure.
 *
 * @name: Signature.
 */
struct bus_resource *
bus_establish(const char *name)
{
    struct bus_resource *brp;
    size_t siglen;

    siglen = strlen(name);
    if (siglen > RSIG_MAX) {
        return NULL;
    }

    /* Allocate a new bus resource handle */
    if ((brp = dynalloc(sizeof(*brp))) == NULL) {
        return NULL;
    }

    memset(brp, 0, sizeof(*brp));
    memcpy(brp->signature, name, siglen);
    if (bus_set(brp, name) < 0) {
        dynfree(brp);
        return NULL;
    }
    return brp;
}

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
#include <sys/sched.h>
#include <vm/fault.h>
#include <vm/map.h>
#include <vm/pmap.h>
#include <vm/vm.h>
#include <vm/physseg.h>

static struct vm_mapping *
vm_mapq_search(vm_mapq_t *mq, vaddr_t addr)
{
    struct vm_mapping *mapping;
    const struct vm_range *range;

    TAILQ_FOREACH(mapping, mq, link) {
        range = &mapping->range;
        if (addr >= range->start && addr <= range->end) {
            return mapping;
        }
    }

    return NULL;
}

static struct vm_mapping *
vm_find_mapping(vaddr_t addr)
{
    struct vm_mapping *mapping;
    struct proc *td = this_td();
    vm_mapq_t *mapq;

    mapping = vm_mapping_fetch(&td->mapspace, addr);
    if (mapping != NULL)
        return mapping;

    /* Need to search other maps */
    for (size_t i = 0; i < MTAB_ENTRIES; ++i) {
        mapq = &td->mapspace.mtab[i];
        mapping = vm_mapq_search(mapq, addr);
        if (mapping != NULL)
            return mapping;
    }

    return NULL;
}

static int
vm_demand_page(struct vm_mapping *mapping, vaddr_t va, vm_prot_t access_type)
{
    struct proc *td;
    paddr_t pa_base;

    int s;
    size_t granule = vm_get_page_size();

    /* Allocate physical memory if needed */
    if (mapping->physmem_base == 0) {
        pa_base = vm_alloc_pageframe(1);
        mapping->physmem_base = pa_base;
    } else {
        pa_base = mapping->physmem_base;
    }

    td = this_td();
    s = vm_map_create(td->addrsp, va, pa_base, access_type, granule);
    return s;
}

int
vm_fault(vaddr_t va, vm_prot_t access_type)
{
    struct vm_mapping *mapping;
    struct vm_object *vmobj;

    int s = 0;
    size_t granule = vm_get_page_size();
    vaddr_t va_base = va & ~(granule - 1);

    mapping = vm_find_mapping(va_base);
    if (mapping == NULL)
        return -1;

    if ((vmobj = mapping->vmobj) == NULL)
        /* Virtual memory object non-existent */
        return -1;
    if ((access_type & ~mapping->prot) != 0)
        /* Invalid access type */
        return -1;

    vm_object_ref(vmobj);

    /* Can we perform demand paging? */
    if (vmobj->demand) {
        s = vm_demand_page(mapping, va_base, access_type);
        if (s != 0)
            goto done;
    }

done:
    /* Drop the vmobj ref */
    vm_object_unref(vmobj);
    return s;
}

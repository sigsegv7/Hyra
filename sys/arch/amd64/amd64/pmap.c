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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <machine/tlb.h>
#include <machine/vas.h>
#include <machine/cpu.h>
#include <machine/cdefs.h>
#include <vm/pmap.h>
#include <vm/physmem.h>
#include <vm/vm.h>
#include <assert.h>
#include <string.h>

/*
 * Page-Table Entry (PTE) flags
 *
 * See Intel SDM Vol 3A, Section 4.5, Table 4-19
 */
#define PTE_ADDR_MASK   0x000FFFFFFFFFF000
#define PTE_P           BIT(0)        /* Present */
#define PTE_RW          BIT(1)        /* Writable */
#define PTE_US          BIT(2)        /* User r/w allowed */
#define PTE_PWT         BIT(3)        /* Page-level write-through */
#define PTE_PCD         BIT(4)        /* Page-level cache disable */
#define PTE_ACC         BIT(5)        /* Accessed */
#define PTE_DIRTY       BIT(6)        /* Dirty (written-to page) */
#define PTE_PAT         BIT(7)
#define PTE_GLOBAL      BIT(8)
#define PTE_NX          BIT(63)       /* Execute-disable */

/*
 * Convert pmap protection flags to PTE flags.
 */
static uint64_t
pmap_prot_to_pte(vm_prot_t prot)
{
    uint64_t pte_flags = PTE_P | PTE_NX;

    if (ISSET(prot, PROT_WRITE))
        pte_flags |= PTE_RW;
    if (ISSET(prot, PROT_EXEC))
        pte_flags &= ~(PTE_NX);
    if (ISSET(prot, PROT_USER))
        pte_flags |= PTE_US;

    return pte_flags;
}

/*
 * Returns index for a specific pagemap level.
 *
 * @level: Requested level.
 * @va: Virtual address.
 */
static size_t
pmap_get_level_index(uint8_t level, vaddr_t va)
{
    __assert(level <= 4 && level != 0);

    switch (level) {
    case 4:
        return (va >> 39) & 0x1FF;
    case 3:
        return (va >> 30) & 0x1FF;
    case 2:
        return (va >> 21) & 0x1FF;
    case 1:
        return (va >> 12) & 0x1FF;
    default:        /* Should not be reachable */
        return 0;
    }
}

/*
 * Extract a pagemap level.
 */
static uintptr_t *
pmap_extract(uint8_t level, vaddr_t va, vaddr_t *pmap, bool alloc)
{
    uintptr_t next, level_alloc;
    size_t idx = pmap_get_level_index(level, va);

    if (pmap == NULL) {
        return NULL;
    }

    if (ISSET(pmap[idx], PTE_P)) {
        next = (pmap[idx] & PTE_ADDR_MASK);
        return PHYS_TO_VIRT(next);
    }

    if (!alloc) {
        return NULL;
    }

    /* Allocate the next level */
    level_alloc = vm_alloc_frame(1);
    if (level_alloc == 0) {
        return NULL;
    }

    memset(PHYS_TO_VIRT(level_alloc), 0, DEFAULT_PAGESIZE);
    pmap[idx] = level_alloc | (PTE_P | PTE_RW | PTE_US);
    return PHYS_TO_VIRT(level_alloc);
}

/*
 * Modify a page table by writing `val' to it.
 *
 * @vas: Virtual address space.
 * @va: Virtual address.
 * @alloc: True to alloc new entries.
 * @res: Result
 */
static int
pmap_get_tbl(struct vas vas, vaddr_t va, bool alloc, uintptr_t **res)
{
    uintptr_t *pml4 = PHYS_TO_VIRT(vas.top_level);
    uintptr_t *pdpt, *pd, *tbl;
    int status = 0;

    pdpt = pmap_extract(4, va, pml4, alloc);
    if (pdpt == NULL) {
        status = 1;
        goto done;
    }

    pd = pmap_extract(3, va, pdpt, alloc);
    if (pd == NULL) {
        status = 1;
        goto done;
    }

    tbl = pmap_extract(2, va, pd, alloc);
    if (tbl == NULL) {
        status = 1;
        goto done;
    }

    *res = tbl;
done:
    return status;
}

/*
 * Update the value in a page table.
 *
 * @vas: Virtual address space.
 * @va: Target virtual address.
 * @val: Value to write.
 * @alloc: True to alloc new paging entries.
 */
static int
pmap_update_tbl(struct vas vas, vaddr_t va, uint64_t val, bool alloc)
{
    uintptr_t *tbl;
    int status;

    if ((status = pmap_get_tbl(vas, va, alloc, &tbl)) != 0) {
        return status;
    }

    tbl[pmap_get_level_index(1, va)] = val;
    tlb_flush(va);
    return 0;
}

int
pmap_new_vas(struct vas *res)
{
    const struct vas *kvas = &g_kvas;
    struct vas new_vas;
    uint64_t *src, *dest;

    new_vas.cr3_flags = kvas->cr3_flags;
    new_vas.top_level = vm_alloc_frame(1);
    if (new_vas.top_level == 0)
        return -ENOMEM;

    src = PHYS_TO_VIRT(kvas->top_level);
    dest = PHYS_TO_VIRT(new_vas.top_level);

    /*
     * Keep the higher half but zero out the lower
     * half for user programs.
     */
    for (int i = 0; i < 512; ++i) {
        if (i < 256) {
            dest[i] = 0;
            continue;
        }

        dest[i] = src[i];
    }

    *res = new_vas;
    return 0;
}

void
pmap_destroy_vas(struct vas vas)
{
    vm_free_frame(vas.top_level, 1);
}

struct vas
pmap_read_vas(void)
{
    struct vas vas;
    uint64_t cr3_raw;

    __ASMV("mov %%cr3, %0"
           : "=r" (cr3_raw)
           :
           : "memory"
    );

    vas.cr3_flags = cr3_raw & ~PTE_ADDR_MASK;
    vas.top_level = cr3_raw & PTE_ADDR_MASK;
    vas.use_l5_paging = false;  /* TODO */
    vas.lock.lock = 0;
    return vas;
}

void
pmap_switch_vas(struct vas vas)
{
    uintptr_t cr3_val = vas.cr3_flags | vas.top_level;

    __ASMV("mov %0, %%cr3"
           :
           : "r" (cr3_val)
           : "memory"
    );
}

int
pmap_map(struct vas vas, vaddr_t va, paddr_t pa, vm_prot_t prot)
{
    uint32_t flags = pmap_prot_to_pte(prot);

    return pmap_update_tbl(vas, va, (pa | flags), true);
}

int
pmap_unmap(struct vas vas, vaddr_t va)
{
    return pmap_update_tbl(vas, va, 0, false);
}

int
pmap_set_cache(struct vas vas, vaddr_t va, int type)
{
    uintptr_t *tbl;
    uint32_t flags;
    paddr_t pa;
    int status;
    size_t idx;

    if ((status = pmap_get_tbl(vas, va, false, &tbl)) != 0)
        return status;

    idx = pmap_get_level_index(1, va);
    pa = tbl[idx] & PTE_ADDR_MASK;
    flags = tbl[idx] & ~PTE_ADDR_MASK;

    /* Set the caching policy */
    switch (type) {
    case VM_CACHE_UC:
        flags |= PTE_PCD;
        flags &= ~PTE_PWT;
        break;
    case VM_CACHE_WT:
        flags &= ~PTE_PCD;
        flags |= PTE_PWT;
        break;
    default:
        return -EINVAL;
    }

    return pmap_update_tbl(vas, va, (pa | flags), false);
}

bool
pmap_is_clean(struct vas vas, vaddr_t va)
{
    uintptr_t *tbl;
    int status;
    size_t idx;

    if ((status = pmap_get_tbl(vas, va, false, &tbl)) != 0)
        return status;

    idx = pmap_get_level_index(1, va);
    return ISSET(tbl[idx], PTE_DIRTY) == 0;
}

void
pmap_mark_clean(struct vas vas, vaddr_t va)
{
    uintptr_t *tbl;
    int status;
    size_t idx;

    if ((status = pmap_get_tbl(vas, va, false, &tbl)) != 0)
        return;

    idx = pmap_get_level_index(1, va);
    tbl[idx] &= ~PTE_DIRTY;

    if (cpu_count() > 1) {
        cpu_shootdown_tlb(va);
    } else {
        __invlpg(va);
    }
}

int
pmap_init(void)
{
    return 0;
}

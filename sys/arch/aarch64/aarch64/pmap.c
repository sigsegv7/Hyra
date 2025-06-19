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
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/panic.h>
#include <machine/vas.h>
#include <vm/pmap.h>
#include <vm/physmem.h>
#include <vm/vm.h>

/* Memory types for MAIR_ELx */
#define MT_NORMAL           0x00
#define MT_NORMAL_UC        0x02
#define MT_DEVICE           0x03

/* Memory attributes */
#define MEM_DEV_NGNRNE  0x00
#define MEM_DEV_NVNRE   0x04
#define MEM_NORMAL_UC   0x44
#define MEM_NORMAL      0xFF

#define MT_ATTR(idx, attr)  ((attr) << (8 * (idx)))

/*
 * Descriptor bits for page table entries
 *
 * @PTE_VALID: Must be set to be valid
 * @PTE_TABLE: Table (1), block (0)
 * @PTE_USER:  User access allowed
 * @PTE_READONLY: Read-only
 * @PTE_ISH: Inner sharable
 * @PTE_AF: Accessed flag
 * @PTE_XN: Execute never
 */
#define PTE_ADDR_MASK 0x0000FFFFFFFFF000
#define PTE_VALID     BIT(0)
#define PTE_TABLE     BIT(1)
#define PTE_USER      BIT(6)
#define PTE_READONLY  BIT(7)
#define PTE_ISH       (3 << 8)
#define PTE_AF        BIT(10)
#define PTE_XN        BIT(54)

/*
 * Write the EL1 Memory Attribute Indirection
 * Register.
 *
 * @val: Value to write
 *
 * XXX: Refer to the ARMv8 Reference Manual section
 *      D7.2.70
 */
static inline void
mair_el1_write(uint64_t val)
{
    __ASMV("msr mair_el1, %0"
           :
           : "r" (val)
           : "memory"
    );
}

static inline void
tlb_flush(vaddr_t va)
{
    __ASMV(
        "tlbi vaae1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :
        : "r" (va >> 12)
        : "memory"
    );
}

static uint64_t
pmap_prot_to_pte(vm_prot_t prot)
{
    uint64_t pte_flags = 0;

    pte_flags |= (PTE_VALID | PTE_TABLE | PTE_AF);
    pte_flags |= (PTE_XN | PTE_READONLY | PTE_ISH);

    if (ISSET(prot, PROT_WRITE))
        pte_flags &= ~PTE_READONLY;
    if (ISSET(prot, PROT_EXEC))
        pte_flags &= ~PTE_XN;
    if (ISSET(prot, PROT_USER))
        pte_flags |= PTE_USER;

    return pte_flags;
}

/*
 * Returns an index for a specific page map
 * label based on an input address.
 */
static size_t
pmap_level_idx(vaddr_t ia, uint8_t level)
{
    switch (level) {
    case 0: return (ia >> 39) & 0x1FF;
    case 1: return (ia >> 30) & 0x1FF;
    case 2: return (ia >> 21) & 0x1FF;
    case 3: return (ia >> 12) & 0x1FF;
    default: panic("pmap_level_idx: bad index\n");
    }

    __builtin_unreachable();
}

/*
 * Extract a level from a pagemap
 *
 * @level: Current pagemap level
 * @ia: Input virtual address
 * @pmap: Current level to extract from
 * @alloc: Set to true to allocate new entries
 *
 * XXX: `level_idx' can be grabbed with pmap_level_idx().
 */
static uintptr_t *
pmap_extract(uint8_t level, vaddr_t ia, vaddr_t *pmap, bool alloc)
{
    uintptr_t next, level_alloc;
    uint8_t idx;

    if (pmap == NULL) {
        return NULL;
    }

    idx = pmap_level_idx(ia, level);
    next = pmap[idx];

    if (ISSET(next, PTE_VALID)) {
        next = next & PTE_ADDR_MASK;
        return PHYS_TO_VIRT(next);
    }

    /*
     * Nothing to grab at this point, we'll need to
     * allocate our own entry. However, if we are
     * told not to allocate anything, just return
     * NULL.
     */
    if (!alloc) {
        return NULL;
    }

    level_alloc = vm_alloc_frame(1);
    if (level_alloc == 0) {
        return NULL;
    }

    pmap[idx] = (level_alloc | PTE_VALID | PTE_USER | PTE_TABLE);
    return PHYS_TO_VIRT(level_alloc);
}

/*
 * Get the lowest pagemap table referring to a 4 KiB
 * frame.
 *
 * @ttrb: Translation table base to use
 * @ia: Input virtual address
 * @alloc: If true, allocate new pagemap entries as needed
 * @res: Result goes here
 */
static int
pmap_get_tbl(paddr_t ttbrn, vaddr_t ia, bool alloc, uintptr_t **res)
{
    vaddr_t *root;
    uintptr_t *l1, *l2, *l3;

    root = PHYS_TO_VIRT(ttbrn);

    l1 = pmap_extract(0, ia, root, alloc);
    if (l1 == NULL) {
        return -1;
    }

    l2 = pmap_extract(1, ia, l1, alloc);
    if (l2 == NULL) {
        return -1;
    }

    l3 = pmap_extract(2, ia, l2, alloc);
    if (l3 == NULL) {
        return -1;
    }

    *res = l3;
    return 0;
}

struct vas
pmap_read_vas(void)
{
    struct vas vas = {0};

    __ASMV(
        "mrs %0, ttbr0_el1\n"
        "mrs %1, ttbr1_el1\n"
        : "=r" (vas.ttbr0_el1),
          "=r" (vas.ttbr1_el1)
        :
        : "memory"
    );

    return vas;
}

void
pmap_switch_vas(struct vas vas)
{
    __ASMV(
        "msr ttbr0_el1, %0\n"
        "msr ttbr1_el1, %1\n"
        :
        : "r" (vas.ttbr0_el1),
          "r" (vas.ttbr1_el1)
        : "memory"
    );
    return;
}

int
pmap_map(struct vas vas, vaddr_t va, paddr_t pa, vm_prot_t prot)
{
    paddr_t ttbrn = vas.ttbr0_el1;
    uint64_t pte_flags;
    uintptr_t *tbl;
    int error;

    if (va >= VM_HIGHER_HALF) {
        ttbrn = vas.ttbr1_el1;
    }

    if ((error = pmap_get_tbl(ttbrn, va, true, &tbl)) < 0) {
        return error;
    }
    if (__unlikely(tbl == NULL)) {
        return -1;
    }

    pte_flags = pmap_prot_to_pte(prot);
    tbl[pmap_level_idx(va, 3)] = pa | pte_flags;
    tlb_flush(va);
    return 0;
}

int
pmap_unmap(struct vas vas, vaddr_t va)
{
    paddr_t ttbrn = vas.ttbr0_el1;
    uintptr_t *tbl;
    int error;

    if (va >= VM_HIGHER_HALF) {
        ttbrn = vas.ttbr1_el1;
    }

    if ((error = pmap_get_tbl(ttbrn, va, true, &tbl)) < 0) {
        return error;
    }
    if (__unlikely(tbl == NULL)) {
        return -1;
    }

    tbl[pmap_level_idx(va, 3)] = 0;
    tlb_flush(va);
    return 0;
}

void
pmap_destroy_vas(struct vas vas)
{
    /* TODO: STUB */
    return;
}

bool
pmap_is_clean(struct vas vas, vaddr_t va)
{
    /* TODO: STUB */
    return false;
}

void
pmap_mark_clean(struct vas vas, vaddr_t va)
{
    /* TODO: STUB */
    return;
}

int
pmap_set_cache(struct vas vas, vaddr_t va, int type)
{
    /* TODO: STUB */
    return 0;
}

int
pmap_init(void)
{
    uint64_t mair;

    mair = MT_ATTR(MT_NORMAL, MEM_NORMAL)       |
           MT_ATTR(MT_NORMAL_UC, MEM_NORMAL_UC) |
           MT_ATTR(MT_DEVICE, MEM_DEV_NGNRNE);
    mair_el1_write(mair);
    return 0;
}

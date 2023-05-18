/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

#include <mm/vm.h>
#include <mm/phys_mgr.h>
#include <arch/aarch64/cpu.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/printk.h>
#include <sys/panic.h>
#include <string.h>

#define PTE_P   (1ull << 0)
#define PTE_TBL (1ull << 1)
#define PTE_U   (1ull << 6)
#define PTE_RO  (1ull << 7)
#define PTE_OSH (2ull << 8)
#define PTE_ISH (3ull << 8)
#define PTE_AF  (1ull << 10)
#define PTE_NG  (1ull << 11)
#define PTE_PXN (1ull << 53)
#define PTE_UXN (1ull << 54)
#define PTE_NX  (PTE_PXN | PTE_UXN)

#define L0_INDEX(virt) ((virt >> 39) & 0x1FF)
#define L1_INDEX(virt) ((virt >> 30) & 0x1FF)
#define L2_INDEX(virt) ((virt >> 21) & 0x1FF)
#define L3_INDEX(virt) ((virt >> 12) & 0x1FF)

MODULE("vm_mmu");

#if defined(__aarch64__)

/*
 * Decides whether to return
 * TTBR0 or TTBR1 based
 * on the virtual address
 * (`virt`) passed.
 */

static uintptr_t
vm_get_ttbrn(struct pagemap pagemap, uintptr_t virt)
{
    if ((virt & 0xFFFFFFFF00000000ULL) != 0) {
        return pagemap.ttbr[1];
    }
 
    return pagemap.ttbr[0];
}

static uintptr_t
vm_get_next_level(uintptr_t level_phys, size_t index,
                  bool alloc, bool *is_block)
{
    uintptr_t *level_virt = (uintptr_t *)(level_phys + VM_HIGHER_HALF);
    uintptr_t entry = 0;

    if ((level_virt[index] & PTE_P) == 0) {
        if (!alloc) {
            return 0;
        }

        level_virt[index] = phys_mgr_alloc(1);
        level_virt[index] |= PTE_P | PTE_TBL;
    }

    entry = level_virt[index];

    if ((entry & PTE_TBL) == 0 && is_block != NULL) {
        *is_block = true;
    } else if (is_block != NULL) {
        *is_block = false;
    }

    return level_virt[index] & ~(0x1FF);
}

/*
 * Returns a region from a virtual address.
 *
 * region.phys_base is set to zero
 * if no valid region was found.
 */

struct vm_region
vm_get_region(struct pagemap pagemap, uintptr_t virt)
{
    bool is_block = false;
    uintptr_t l0 = vm_get_ttbrn(pagemap, virt);
    uintptr_t l1 = vm_get_next_level(l0, L0_INDEX(virt), false, &is_block);

    struct vm_region region = { 0 };
    region.virt_base = virt;
    
    if (l1 == 0) {
        return region;
    } else if (is_block) {
        region.pagesize = PAGESIZE_1GB;
        region.phys_base = l1;
        return region;
    }

    uintptr_t l2 = vm_get_next_level(l1, L1_INDEX(virt), false, &is_block);
    
    if (l2 == 0) {
        return region;
    } else if (is_block) {
        region.pagesize = PAGESIZE_2MB;
        region.phys_base = l2;
        return region;
    }

    uintptr_t l3 = vm_get_next_level(l2, L2_INDEX(virt), false, NULL);
    uintptr_t *l3_virt = (uintptr_t *)(l3 + VM_HIGHER_HALF);

    region.phys_base = l3_virt[L3_INDEX(virt)] & ~(0x1FF);
    region.pagesize = PAGESIZE_4K;
    return region;
}

inline struct pagemap
vm_get_pagemap(void)
{
    struct pagemap pagemap;

    __asm("mrs %0, ttbr0_el1\n"
          "mrs %1, ttbr1_el1\n"
          : "=r" (pagemap.ttbr[0]),
            "=r" (pagemap.ttbr[1])
          :
          : "memory"
    );

    return pagemap;
}

static inline void
vm_set_pagemap(struct pagemap pagemap)
{
    __asm("msr ttbr0_el1, %0\n"
          "msr ttbr1_el1, %1\n"
          :
          : "r" (pagemap.ttbr[0]),
            "r" (pagemap.ttbr[1])
          : "memory"
    );
}

__weak void
vm_init(void)
{
    struct pagemap pagemap = vm_get_pagemap();
    size_t id_mmfr0 = cpu_read_sysreg(id_aa64mmfr0_el1);

    const char *pa_size_map[] = {
        [0x00] = "32-bit",
        [0x01] = "36-bit",
        [0x02] = "40-bit",
        [0x03] = "42-bit",
        [0x04] = "44-bit",
        [0x05] = "48-bit",
        [0x06] = "52-bit"
    };

    kinfo("CPU supports %s physical addresses\n",
          pa_size_map[id_mmfr0 & 0xF]);
    
    if ((id_mmfr0 & (2 << 4)) == 0)
        /* TODO: Add support for 8-bit ASIDs */
        panic("CPU does not support 16-bit ASIDs\n");
    if ((id_mmfr0 & (0xF << 28)) != 0)
        panic("CPU does not support 4K granule\n");
 
    /* Memory Attribute Indirection Register (MAIR) value */
    size_t mair = (0xFF << 0)  |  /* Normal: Write-back, RW-Allocate, non-transient */
                  (0x0C << 8)  |  /* Framebuffer memory */
                  (0x00 << 16) |  /* Device memory: nGnRnE */
                  (0x04 << 24);   /* Normal: Uncachable */
    
    /* Translation Control Register (TCR) value */
    size_t tcr = (16 << 0)      | /* T0SZ=16 */
                 (16 << 16)     | /* T1SZ=16 */
                 (1 << 8)       | /* TTBR0 Inner: WB RW-Allocate */
                 (1 << 10)      | /* TTBR0 Outer: WB RW-Allocate */
                 (1 << 12)      | /* TTBR1 Inner: sharable */
                 (1 << 24)      | /* TTBR1 Inner: WB RW-Allocate */
                 (1 << 26)      | /* TTBR1 Outer: WB RW-Allocate */
                 (1 << 28)      | /* TTBR1 Inner: sharable */
                 (2ULL << 30)   | /* TTBR1: 4K granule */
                 (1ULL << 36);    /* 16-bit ASIDs */

    kinfo("Initializing MMU...\n");
 
    /* TTBR0 is used for userspace so make our own */
    pagemap.ttbr[0] = phys_mgr_alloc(1);

    /* Zero it so we can have nothing mapped in the lower half */
    memset((void *)(pagemap.ttbr[0] + VM_HIGHER_HALF), 0, 0x1000);
    
    cpu_write_sysreg(mair_el1, mair);
    cpu_write_sysreg(tcr_el1, tcr);
    vm_set_pagemap(pagemap);

    kinfo("MMU init finished\n");
    printk("... MAIR=0x%x, TCR=0x%x\n\n", mair, tcr);
}

#endif

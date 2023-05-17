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
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/printk.h>
#include <string.h>

MODULE("vm_mmu");

#if defined(__aarch64__)

static inline void
vm_set_mair(size_t val)
{
    __asm("msr mair_el1, %0"
          :
          : "r" (val)
          : "memory"
    );
}

static inline void
vm_set_tcr(size_t val)
{
    __asm("msr tcr_el1, %0"
          :
          : "r" (val)
          : "memory"
    );
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

    vm_set_mair(mair);
    vm_set_tcr(tcr);
    vm_set_pagemap(pagemap);

    kinfo("MMU init finished\n");
    printk("... MAIR=0x%x, TCR=0x%x\n\n", mair, tcr);
}

#endif

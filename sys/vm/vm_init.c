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

#include <vm/vm.h>
#include <vm/physseg.h>
#include <vm/pmap.h>
#include <sys/panic.h>
#include <assert.h>

/*
 * XXX: As of now, these are constant... It would be nice if we could
 *      have this value expanded upon request so we can keep it small
 *      to not hog up resources. Would make it more flexible too :^)
 */
#define DYNALLOC_POOL_SZ        0x400000  /* 4 MiB */
#define DYNALLOC_POOL_PAGES     (DYNALLOC_POOL_SZ / vm_get_page_size())

static volatile struct vas kernel_vas;

/*
 * TODO: Move this to a per CPU structure, this kinda sucks
 *       how it is right now...
 */
static struct vm_ctx bsp_vm_ctx = {0};

volatile struct limine_hhdm_request g_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

struct vm_ctx *
vm_get_ctx(void)
{
    return &bsp_vm_ctx;
}

/*
 * Return the kernel VAS.
 */
struct vas
vm_get_kvas(void)
{
    return kernel_vas;
}

void
vm_init(void)
{
    void *pool_va;

    kernel_vas = pmap_read_vas();
    if (pmap_init(vm_get_ctx()) != 0) {
        panic("Failed to init pmap layer\n");
    }

    /* Setup virtual memory context */
    bsp_vm_ctx.dynalloc_pool_sz = DYNALLOC_POOL_SZ;
    bsp_vm_ctx.dynalloc_pool_phys = vm_alloc_pageframe(DYNALLOC_POOL_PAGES);
    if (bsp_vm_ctx.dynalloc_pool_phys == 0) {
        panic("Failed to allocate dynamic pool\n");
    }
    pool_va = PHYS_TO_VIRT(bsp_vm_ctx.dynalloc_pool_phys);
    bsp_vm_ctx.tlsf_ctx = tlsf_create_with_pool(pool_va, DYNALLOC_POOL_SZ);
    __assert(bsp_vm_ctx.tlsf_ctx != 0);
}

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

#include <sys/limine.h>
#include <sys/panic.h>
#include <vm/vm.h>
#include <vm/physmem.h>
#include <vm/pmap.h>
#include <assert.h>

#define DYNALLOC_POOL_SZ        0x400000  /* 4 MiB */
#define DYNALLOC_POOL_PAGES     (DYNALLOC_POOL_SZ / DEFAULT_PAGESIZE)

struct vas g_kvas;
static struct vm_ctx vm_ctx;
volatile struct limine_hhdm_request g_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

struct vm_ctx *
vm_get_ctx(void)
{
    return &vm_ctx;
}

void
vm_init(void)
{
    void *pool;

    vm_physmem_init();
    pmap_init();

    g_kvas = pmap_read_vas();
    vm_ctx.dynalloc_pool_sz = DYNALLOC_POOL_SZ;
    vm_ctx.dynalloc_pool_pa = vm_alloc_frame(DYNALLOC_POOL_PAGES);
    if (vm_ctx.dynalloc_pool_pa == 0) {
        panic("failed to allocate dynamic pool\n");
    }

    pool = PHYS_TO_VIRT(vm_ctx.dynalloc_pool_pa);
    vm_ctx.tlsf_ctx = tlsf_create_with_pool(pool, DYNALLOC_POOL_SZ);
    __assert(vm_ctx.tlsf_ctx != 0);
}

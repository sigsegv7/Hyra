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

#ifndef _VM_H_
#define _VM_H_

#include <sys/types.h>
#include <sys/limine.h>
#include <sys/spinlock.h>
#include <vm/tlsf.h>
#include <vm/pmap.h>

extern volatile struct limine_hhdm_request g_hhdm_request;

#define VM_HIGHER_HALF (g_hhdm_request.response->offset)
#define PHYS_TO_VIRT(phys) (void *)((uintptr_t)phys + VM_HIGHER_HALF)
#define VIRT_TO_PHYS(virt) ((uintptr_t)virt - VM_HIGHER_HALF)

#define DEFAULT_PAGESIZE 4096

struct vm_ctx {
    size_t dynalloc_pool_sz;
    uintptr_t dynalloc_pool_pa;
    struct spinlock dynalloc_lock;
    tlsf_t tlsf_ctx;
};

extern struct vas g_kvas;

struct vm_ctx *vm_get_ctx(void);
void vm_init(void);

#endif

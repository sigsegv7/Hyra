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

/*
 * For documentation: See vm(9)
 */

#ifndef _VM_H_
#define _VM_H_

#include <sys/types.h>
#include <sys/limine.h>
#include <sys/cdefs.h>
#include <vm/page.h>
#include <vm/pmap.h>
#include <vm/physseg.h>

extern volatile struct limine_hhdm_request g_hhdm_request;

#define VM_HIGHER_HALF (g_hhdm_request.response->offset)

#define PHYS_TO_VIRT(phys) (void *)((uintptr_t)phys + VM_HIGHER_HALF)
#define VIRT_TO_PHYS(virt) ((uintptr_t)virt - VM_HIGHER_HALF)

struct vm_range {
    uintptr_t start;
    uintptr_t end;
};

struct vm_memstat {
    struct physmem_stat pmem_stat;
    size_t vmobj_cnt;
};

/*
 * Returns the machine's pagesize:
 *
 * XXX TODO: This needs to be moved to vmm_init.c
 *           while returning a non-constant value.
 */
static inline size_t
vm_get_page_size(void)
{
    return 4096;
}

void vm_init(void);
struct vm_ctx *vm_get_ctx(void);
struct vas vm_get_kvas(void);
struct vm_memstat vm_memstat(void);

#endif      /* !_VM_H_ */

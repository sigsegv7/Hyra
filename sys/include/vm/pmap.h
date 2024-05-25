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

#ifndef _VM_PMAP_H_
#define _VM_PMAP_H_

/*
 * Each architecture is expected to implement
 * this header. It should contain a `struct vas'
 * which will contain information about a VAS
 * (virtual address space)
 *
 * On AMD64 this struct contains: PML4, etc
 *
 * XXX: Compiler errors pointing to this include means you
 *      forgot to implement it!!!
 *
 *      `struct vas' MUST have a `struct spinlock lock' field!!!
 */
#include <machine/vas.h>

#include <vm/tlsf.h>
#include <sys/types.h>
#include <sys/spinlock.h>

/* prot flags for mappings */
#define PROT_WRITE      __BIT(0)      /* Writable */
#define PROT_EXEC       __BIT(1)      /* Executable */
#define PROT_USER       __BIT(2)      /* User accessible */

#define is_vas_valid(vas) (vas.top_level != 0)

/*
 * vm_ctx - Per core virtual memory context
 */
struct vm_ctx {
    uintptr_t dynalloc_pool_phys;
    size_t dynalloc_pool_sz;    /* In bytes */
    tlsf_t tlsf_ctx;
    struct spinlock dynalloc_lock;
};

/*
 * Create a virtual address space
 * and return the descriptor.
 */
int pmap_create_vas(struct vm_ctx *, struct vas *);

/*
 * Switch the current virtual address space
 * to another.
 */
void pmap_switch_vas(struct vm_ctx *, struct vas);

/*
 * Read virtual address space descriptor
 * and return it.
 */
struct vas pmap_read_vas(void);

/*
 * Map a physical address to a virtual address.
 */
int pmap_map(struct vm_ctx *, struct vas, vaddr_t, paddr_t, vm_prot_t);

/*
 * Get rid of a virtual address space and free
 * resources.
 */
int pmap_free_vas(struct vm_ctx *, struct vas);

/*
 * Unmap a page.
 */
int pmap_unmap(struct vm_ctx *, struct vas, vaddr_t);

/*
 * Architecture specific init code for pmap
 */
int pmap_init(struct vm_ctx *);
#endif  /* _VM_PMAP_H_ */

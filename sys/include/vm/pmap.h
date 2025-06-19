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

#ifndef _VM_PMAP_H_
#define _VM_PMAP_H_

#include <sys/types.h>
#include <machine/vas.h>

/* Protection flags for mappings */
#define PROT_READ       0x00000000UL
#define PROT_WRITE      BIT(0)      /* Writable */
#define PROT_EXEC       BIT(1)      /* Executable */
#define PROT_USER       BIT(2)      /* User accessible */

/* Caching types */
#define VM_CACHE_UC 0x00000U /* Uncachable */
#define VM_CACHE_WT 0x00001U /* Write-through */

typedef uint32_t vm_prot_t;

/*
 * Fetch the current address space.
 */
struct vas pmap_read_vas(void);

/*
 * Switch the virtual address space.
 */
void pmap_switch_vas(struct vas vas);

/*
 * Create a new virtual address space.
 */
int pmap_new_vas(struct vas *res);

/*
 * Deallocate a virtual address space.
 */
void pmap_destroy_vas(struct vas vas);

/*
 * Create a virtual memory mapping of a single page.
 */
int pmap_map(struct vas vas, vaddr_t va, paddr_t pa, vm_prot_t prot);

/*
 * Unmap a virtual memory mapping of a single page.
 */
int pmap_unmap(struct vas vas, vaddr_t va);

/*
 * Returns true if the page is clean (modified), otherwise
 * returns false.
 */
bool pmap_is_clean(struct vas vas, vaddr_t va);

/*
 * Marks a page as clean (unmodified)
 */
void pmap_mark_clean(struct vas vas, vaddr_t va);

/*
 * Mark a virtual address with a specific
 * caching type.
 */
int pmap_set_cache(struct vas vas, vaddr_t va, int type);

/*
 * Machine dependent pmap init code.
 */
int pmap_init(void);

#endif  /* !_VM_PMAP_H_ */

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

#ifndef _VM_MMAP_H_
#define _VM_MMAP_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/syscall.h>
#include <sys/spinlock.h>
#include <vm/pmap.h>
#include <vm/vm.h>

#define MAP_SHARED      0x0001
#define MAP_PRIVATE     0x0002
#define MAP_ANONYMOUS   0x0010
#define MAP_FAILED      ((void *)-1)

/* Memory map table entry count */
#define MTAB_ENTRIES 32

struct vm_object;

struct vm_mapping {
    TAILQ_ENTRY(vm_mapping) link;
    struct vm_range range;
    struct vm_object *vmobj;
    paddr_t physmem_base;
    vm_prot_t prot;

    /* Private */
    size_t vhash;           /* Virtual address hash */
};

typedef TAILQ_HEAD(, vm_mapping) vm_mapq_t;

struct vm_mapspace {
    vm_mapq_t mtab[MTAB_ENTRIES];   /* Map table */
    size_t map_count;
};

/* Mapping operations */
int vm_map_destroy(struct vas vas, vaddr_t va, size_t bytes);
int vm_map_create(struct vas vas, vaddr_t va, paddr_t pa, vm_prot_t prot,
                  size_t bytes);

/* Syscalls */
uint64_t sys_mmap(struct syscall_args *args);
uint64_t sys_munmap(struct syscall_args *args);

/* Mapespace operations */
void vm_mapspace_insert(struct vm_mapspace *ms, struct vm_mapping *mapping);
void vm_mapspace_remove(struct vm_mapspace *ms, struct vm_mapping *mapping);
struct vm_mapping *vm_mapping_fetch(struct vm_mapspace *ms, vaddr_t va);
void vm_free_mapq(vm_mapq_t *mapq);

#endif  /* !_VM_MMAP_H_ */

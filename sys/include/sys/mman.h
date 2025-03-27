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

#ifndef _SYS_MMAN_H_
#define _SYS_MMAN_H_

#include <sys/types.h>
#include <sys/syscall.h>
#if defined(_KERNEL)
#include <sys/tree.h>
#include <vm/vm_obj.h>
#endif  /* _KERNEL */

/*
 * If we are in kernel space, all defines we want are
 * in vm/pmap.h
 */
#if !defined(_KERNEL)
#define PROT_WRITE 0x00000001
#define PROT_EXEC  0x00000002
#define PROT_NONE  0x00000004
#define PROT_READ  PROT_NONE
#endif  /* !_KERNEL */

/* mmap() flags */
#define MAP_SHARED  0x0001
#define MAP_PRIVATE 0x0002
#define MAP_FIXED   0x0004
#define MAP_ANON    0x0008

#if defined(_KERNEL)
/*
 * The mmap ledger entry
 *
 * @va_start: Starting virtual address.
 * @obj: VM object representing this entry.
 */
struct mmap_entry {
    vaddr_t va_start;
    struct vm_object *obj;
    size_t size;
    RBT_ENTRY(mmap_entry) hd;
};

/*
 * The mmap ledger is a per-process structure that
 * describes memory mappings made using mmap()
 *
 * @hd: Red-black tree of mmap_entry structures
 * @nbytes: Total bytes mapped.
 */
struct mmap_lgdr {
    RBT_HEAD(lgdr_entries, mmap_entry) hd;
    size_t nbytes;
};

/* Kernel munmap() routine */
int munmap_at(void *addr, size_t len);

/* Kernel mmap() routine */
void *mmap_at(void *addr, size_t len, int prot, int flags,
              int fildes, off_t off);

int mmap_entrycmp(const struct mmap_entry *a, const struct mmap_entry *b);
RBT_PROTOTYPE(lgdr_entries, mmap_entry, hd, mmap_entrycmp)
#endif  /* _KERNEL */

/* Syscall layer */
scret_t mmap(struct syscall_args *scargs);
scret_t munmap(struct syscall_args *scargs);

#endif  /* !_SYS_MMAN_H_ */

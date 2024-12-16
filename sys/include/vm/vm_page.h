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

#ifndef _VM_PAGE_H_
#define _VM_PAGE_H_

#include <sys/types.h>
#include <sys/tree.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/spinlock.h>

struct vm_object;

/*
 * Represents a single virtual memory page.
 */
struct vm_page {
    TAILQ_ENTRY(vm_page) pageq;     /* Queue data */
    RBT_ENTRY(vm_page) objt;        /* Object tree */
    paddr_t phys_addr;              /* Physical address of page */
    struct spinlock lock;           /* Page lock */
    uint32_t flags;                 /* Page flags */
    off_t offset;                   /* Offset into object */
};

/* Page flags */
#define PG_VALID    BIT(0)      /* Has to be set to be valid */
#define PG_CLEAN    BIT(1)      /* Page has not be written to */
#define PG_REQ      BIT(2)      /* Page has been requested by someone */

/* Page alloc flags */
#define PALLOC_ZERO BIT(0)

struct vm_page *vm_pagelookup(struct vm_object *obj, off_t off);
struct vm_page *vm_pagealloc(struct vm_object *obj, int flags);
void vm_pagefree(struct vm_object *obj, struct vm_page *pg, int flags);

#endif  /* !_VM_PAGE_H_ */

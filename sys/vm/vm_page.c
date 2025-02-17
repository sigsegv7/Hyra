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

#include <vm/vm_page.h>
#include <vm/vm_obj.h>
#include <vm/physmem.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <assert.h>
#include <string.h>

RBT_GENERATE(vm_objtree, vm_page, objt, vm_pagecmp);

/*
 * Insert a page into an object.
 */
static inline void
vm_pageinsert(struct vm_page *pg, struct vm_object *obp)
{
   struct vm_page *tmp;

   tmp = RBT_INSERT(vm_objtree, &obp->objt, pg);
   __assert(tmp == NULL);
   ++obp->npages;
}

static inline void
vm_pageremove(struct vm_page *pg, struct vm_object *obp)
{
    RBT_REMOVE(vm_objtree, &obp->objt, pg);
    --obp->npages;
}

struct vm_page *
vm_pagelookup(struct vm_object *obj, off_t off)
{
    struct vm_page tmp;

    tmp.offset = off;
    return RBT_FIND(vm_objtree, &obj->objt, &tmp);
}

struct vm_page *
vm_pagealloc(struct vm_object *obj, int flags)
{
    struct vm_page *tmp;

    tmp = dynalloc(sizeof(*tmp));
    if (tmp == NULL) {
        return NULL;
    }

    memset(tmp, 0, sizeof(*tmp));
    tmp->phys_addr = vm_alloc_frame(1);
    tmp->flags |= (PG_VALID | PG_CLEAN);
    __assert(tmp->phys_addr != 0);

    if (ISSET(flags, PALLOC_ZERO)) {
        memset(PHYS_TO_VIRT(tmp->phys_addr), 0, DEFAULT_PAGESIZE);
    }

    vm_pageinsert(tmp, obj);
    return tmp;
}

void
vm_pagefree(struct vm_object *obj, struct vm_page *pg, int flags)
{
    __assert(pg->phys_addr != 0);

    vm_pageremove(pg, obj);
    vm_free_frame(pg->phys_addr, 1);
    dynfree(pg);
}

int
vm_pagecmp(const struct vm_page *a, const struct vm_page *b)
{
    return (a->offset < b->offset) ? -1 : a->offset > b->offset;
}

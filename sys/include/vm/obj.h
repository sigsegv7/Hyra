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

#ifndef _VM_OBJ_H_
#define _VM_OBJ_H_

#include <sys/spinlock.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <vm/map.h>
#include <vm/pager.h>

struct vm_object {
    struct spinlock lock;           /* Protects this object */
    struct vm_mapspace mapspace;    /* Mapspace this object points to */
    struct vm_pagerops *pgops;      /* Pager operations */

    uint8_t is_anon : 1;            /* Is an anonymous mapping */
    uint8_t demand  : 1;            /* Only mapped upon access */
    int ref;                        /* Ref count */
    struct vnode *vnode;            /* Only used if `is_anon` is 0 */
};

#define vm_object_ref(OBJPTR) (++(OBJPTR)->ref)
#define vm_object_unref(OBJPTR) do {    \
        if ((OBJPTR)->ref > 1) {        \
            --(OBJPTR)->ref;            \
        }                               \
    } while (0);

int vm_obj_init(struct vm_object **res, struct vnode *vnode);
int vm_obj_destroy(struct vm_object *obj);

#endif  /* !_VM_OBJ_H_ */

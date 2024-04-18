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

#include <vm/obj.h>
#include <vm/dynalloc.h>
#include <sys/errno.h>
#include <string.h>

static void
vm_set_pgops(struct vm_object *obj, struct vnode *vnode)
{
    if (vnode == NULL) {
        obj->vnode = NULL;
        return;
    }

    /* Is this a device? */
    if (vnode->type == VCHR || vnode->type == VBLK) {
        obj->pgops = &g_dev_pagerops;
    } else {
        obj->pgops = &g_vnode_pagerops;
    }
}

int
vm_obj_init(struct vm_object **res, struct vnode *vnode)
{
    struct vm_object *obj = dynalloc(sizeof(struct vm_object));

    if (obj == NULL) {
        return -ENOMEM;
    }

    memset(obj, 0, sizeof(struct vm_object));
    obj->vnode = vnode;

    vm_set_pgops(obj, vnode);
    *res = obj;
    return 0;
}

int
vm_obj_destroy(struct vm_object *obj)
{
    struct vnode *vp = obj->vnode;

    if (vp->vmobj != NULL)
        vp->vmobj = NULL;

    dynfree(obj);
    return 0;
}

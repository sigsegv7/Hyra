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

#ifndef _SYS_VNODE_H_
#define _SYS_VNODE_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/mount.h>
#include <vm/obj.h>
#include <sys/sio.h>

struct vnode;
struct vattr;

struct vops {
    int(*vget)(struct vnode *parent, const char *name, struct vnode **vp);
    int(*read)(struct vnode *vp, struct sio_txn *sio);
    int(*write)(struct vnode *vp, struct sio_txn *sio);
    int(*getattr)(struct vnode *vp, struct vattr *vattr);
    int(*open)(struct vnode *vp);
    int(*close)(struct vnode *vp);
};

struct vattr {
    size_t size;        /* File size in bytes */
    int type;           /* Vnode type */
};

struct vnode {
    int type;
    int flags;
    struct vm_object *vmobj;
    struct mount *mp;   /* Ptr to vfs vnode is in */
    struct vops *vops;
    struct vnode *parent;
    struct fs_info *fs; /* Filesystem this vnode belongs to, can be NULL */
    void *data;         /* Filesystem specific data */
};

/*
 * Vnode type flags
 */
#define VREG    0x01    /* Regular file */
#define VDIR    0x02    /* Directory */
#define VCHR    0x03    /* Character device */
#define VBLK    0x04    /* Block device */

#if defined(_KERNEL)
int vfs_alloc_vnode(struct vnode **vnode, struct mount *mp, int type);
#endif

#endif

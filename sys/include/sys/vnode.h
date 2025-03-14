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

#ifndef _SYS_VNODE_H_
#define _SYS_VNODE_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/atomic.h>
#include <sys/sio.h>
#include <vm/vm_obj.h>

#if defined(_KERNEL)

struct vops;

struct vnode {
    int type;
    int flags;
    void *data;
    const struct vops *vops;
    struct vm_object vobj;
    uint32_t refcount;
    TAILQ_ENTRY(vnode) vcache_link;
};

/*
 * Vnode cache, can be per-process or
 * global.
 */
struct vcache {
    TAILQ_HEAD(vcache_head, vnode) q;
    ssize_t size;    /* In entries (-1 not set up) */
};

#define vfs_vref(VP) (atomic_inc_int(&(VP)->refcount))

/* vcache types */
#if defined(_KERNEL)
#define VCACHE_TYPE_NONE   0
#define VCACHE_TYPE_PROC   1
#define VCACHE_TYPE_GLOBAL 2
#endif  /* KERNEL */

/* Vnode type flags */
#define VNON    0x00    /* Uninitialized */
#define VREG    0x01    /* Regular file */
#define VDIR    0x02    /* Directory */
#define VCHR    0x03    /* Character device */
#define VBLK    0x04    /* Block device */

#define VNOVAL -1

struct vop_lookup_args {
    const char *name;       /* Current path component */
    struct vnode *dirvp;    /* Directory vnode */
    struct vnode **vpp;     /* Result vnode */
};

/*
 * A field in this structure is unavailable
 * if it has a value of VNOVAL.
 */
struct vattr {
    mode_t mode;
    size_t size;
};

struct vop_getattr_args {
    struct vnode *vp;
    struct vattr *res;
};

struct vops {
    int(*lookup)(struct vop_lookup_args *args);
    int(*getattr)(struct vop_getattr_args *args);
    int(*read)(struct vnode *vp, struct sio_txn *sio);
    int(*write)(struct vnode *vp, struct sio_txn *sio);
    int(*reclaim)(struct vnode *vp);
};

extern struct vnode *g_root_vnode;

int vfs_vcache_type(void);
int vfs_vcache_migrate(int newtype);

int vfs_vcache_enter(struct vnode *vp);
struct vnode *vfs_recycle_vnode(void);

int vfs_alloc_vnode(struct vnode **res, int type);
int vfs_release_vnode(struct vnode *vp);

int vfs_vop_lookup(struct vnode *vp, struct vop_lookup_args *args);
int vfs_vop_read(struct vnode *vp, struct sio_txn *sio);
int vfs_vop_write(struct vnode *vp, struct sio_txn *sio);
int vfs_vop_getattr(struct vnode *vp, struct vop_getattr_args *args);

#endif  /* _KERNEL */
#endif  /* !_SYS_VNODE_H_ */

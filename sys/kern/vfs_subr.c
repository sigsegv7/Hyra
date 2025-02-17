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

#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/mount.h>
#include <sys/syslog.h>
#include <vm/dynalloc.h>
#include <string.h>

mountlist_t g_mountlist;

int
vfs_alloc_vnode(struct vnode **res, int type)
{
    struct vnode *vp = vfs_recycle_vnode();

    /*
     * If there are no vnodes to be recycled, attempt
     * to allocate a new one.
     */
    if (vp == NULL)
        vp = dynalloc(sizeof(struct vnode));
    if (vp == NULL)
        return -ENOMEM;

    memset(vp, 0, sizeof(*vp));
    vp->type = type;
    vp->refcount = 1;
    *res = vp;
    return 0;
}

/*
 * Allocate a mount structure.
 *
 * @vp: Vnode this mount structure covers.
 * @fip: File system information.
 */
struct mount *
vfs_alloc_mount(struct vnode *vp, struct fs_info *fip)
{
    struct mount *mp;

    mp = dynalloc(sizeof(*mp));
    if (mp == NULL)
        return NULL;

    memset(mp, 0, sizeof(*mp));
    mp->vp = vp;
    mp->mnt_ops = fip->vfsops;
    return mp;
}

/*
 * Assign a name to a mountpoint.
 *
 * @mp: Mountpoint to name.
 * @name: Name to give.
 */
int
vfs_name_mount(struct mount *mp, const char *name)
{
    size_t name_len = strlen(name);

    if (name_len > NAME_MAX)
        return -ENAMETOOLONG;

    mp->name = dynalloc(sizeof(char) * name_len + 1);
    if (mp->name == NULL)
        return -ENOMEM;

    memcpy(mp->name, name, name_len);
    mp->name[name_len] = '\0';
    return 0;
}

/*
 * Release the resources associated with a vnode and
 * mark the vnode to *possibly* be deallocated unless
 * recycled.
 */
int
vfs_release_vnode(struct vnode *vp)
{
    const struct vops *vops = vp->vops;
    int status = 0;

    if (vp == NULL)
        return -EINVAL;

    /*
     * The refcount cannot be zero before we decrement it,
     * something is quite wrong if this happens.
     */
    if (vp->refcount == 0) {
        kprintf("Cannot release vnode, bad refcount\n");
        return -EIO;
    }

    /*
     * Drop the reference and don't destroy the vnode
     * if it's still not zero.
     */
    if (atomic_dec_int(&vp->refcount) > 0)
        return 0;

    if (vops->reclaim != NULL)
        status = vops->reclaim(vp);
    if (status != 0)
        return status;

    vfs_vcache_enter(vp);
    return status;
}

int
vfs_vop_lookup(struct vnode *vp, struct vop_lookup_args *args)
{
    const struct vops *vops = vp->vops;

    if (vops == NULL)
        return -EIO;
    if (vops->lookup == NULL)
        return -EIO;

    return vops->lookup(args);
}

int
vfs_vop_read(struct vnode *vp, struct sio_txn *sio)
{
    const struct vops *vops = vp->vops;

    if (vops == NULL)
        return -EIO;
    if (vops->read == NULL)
        return -EIO;

    return vops->read(vp, sio);
}

int
vfs_vop_getattr(struct vnode *vp, struct vop_getattr_args *args)
{
    const struct vops *vops = vp->vops;

    if (vops == NULL)
        return -EIO;
    if (vops->getattr == NULL)
        return -EIO;

    return vops->getattr(args);
}

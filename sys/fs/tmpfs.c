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

#include <sys/mount.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/panic.h>
#include <sys/vnode.h>
#include <vm/dynalloc.h>
#include <vm/vm_obj.h>
#include <vm/vm_page.h>
#include <vm/vm_pager.h>
#include <fs/tmpfs.h>
#include <string.h>

#define ROOT_RPATH "/tmp"
#define TMPFS_BSIZE DEFAULT_PAGESIZE

#define pr_trace(fmt, ...) kprintf("tmpfs: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static TAILQ_HEAD(, tmpfs_node) root;

/*
 * Generate a vnode for a specific tmpfs
 * node.
 */
static int
tmpfs_ref(struct tmpfs_node *np)
{
    struct vnode *vp = NULL;
    int retval = 0;

    if (np->vp == NULL) {
        spinlock_acquire(&np->lock);
        retval = vfs_alloc_vnode(&vp, np->type);
        np->vp = vp;
        spinlock_release(&np->lock);
    }

    if (vp != NULL) {
        vp->data = np;
        vp->vops = &g_tmpfs_vops;
    }

    return retval;
}

/*
 * Perform lookup within the tmpfs namespace
 *
 * XXX: This operations is serialized
 * TODO: Support multiple directories (only fs root now)
 *
 * @rpath: /tmp/ relative path to lookup
 * @res: The result is written here (must NOT be NULL)
 */
static int
tmpfs_do_lookup(const char *rpath, struct tmpfs_node **res)
{
    struct tmpfs_node *cnp;
    struct tmpfs_node *dirent;
    int error = 0;

    /*
     * If the directory is the node that we are
     * looking for, return it. But if it is not
     * and it is empty then there is nothing
     * we can do.
     */
    cnp = TAILQ_FIRST(&root);
    if (strcmp(cnp->rpath, rpath) == 0) {
        *res = cnp;
        return 0;
    }
    if (TAILQ_NELEM(&cnp->dirents) == 0) {
        return -ENOENT;
    }

    /*
     * Go through each tmpfs dirent to see if we can
     * find the file we are looking for.
     */
    spinlock_acquire(&cnp->lock);
    dirent = TAILQ_FIRST(&cnp->dirents);
    while (dirent != NULL) {
        if (strcmp(dirent->rpath, rpath) == 0) {
            break;
        }

        dirent = TAILQ_NEXT(dirent, link);
    }

    spinlock_release(&cnp->lock);
    if (dirent == NULL) {
        return -ENOENT;
    }

    if ((error = tmpfs_ref(dirent)) != 0) {
        return error;
    }

    *res = dirent;
    return 0;
}

/*
 * TMPFS lookup callback for the VFS
 *
 * Takes some arguments and returns a vnode
 * in args->vpp
 */
static int
tmpfs_lookup(struct vop_lookup_args *args)
{
    struct tmpfs_node *np;
    int error;

    if (args == NULL) {
        return -EINVAL;
    }
    if (args->name == NULL) {
        return -EINVAL;
    }

    /*
     * Attempt to find the node we want, if it already
     * has a vnode attached to it then that's something we
     * want. However we should allocate a new vnode if we
     * need to.
     */
    error = tmpfs_do_lookup(args->name, &np);
    if (error != 0) {
        return error;
    }

    *args->vpp = np->vp;
    return 0;
}

/*
 * TMPFS create callback for the VFS
 *
 * Creates a new TMPFS node
 */
static int
tmpfs_create(struct vop_create_args *args)
{
    const char *pcp = args->path;   /* Stay away from boat, kids */
    struct vnode *dirvp;
    struct tmpfs_node *np;
    struct tmpfs_node *root_np;
    int error;

    /* Validate inputs */
    if (args == NULL)
        return -EINVAL;
    if (pcp == NULL)
        return -EIO;
    if ((dirvp = args->dirvp) == NULL)
        return -EIO;

    /* Remove the leading "/tmp/" */
    pcp += sizeof(ROOT_RPATH);
    if (*pcp == '\0') {
        return -ENOENT;
    }

    np = dynalloc(sizeof(*np));
    if (np == NULL) {
        return -ENOMEM;
    }

    memset(np, 0, sizeof(*np));

    /*
     * TODO: Support multiple directories.
     *
     * XXX: We currently only create a TMPFS_REG node as
     *      to keep things initially simple.
     */
    root_np = TAILQ_FIRST(&root);
    np->dirvp = dirvp;
    np->type = TMPFS_REG;
    np->real_size = 0;
    np->mode = 0700;
    memcpy(np->rpath, pcp, strlen(pcp) + 1);
    TAILQ_INSERT_TAIL(&root_np->dirents, np, link);

    if ((error = tmpfs_ref(np)) != 0) {
        return error;
    }

    *args->vpp = np->vp;
    return 0;
}

/*
 * TMPFS write callback for VFS
 *
 * Node buffers are orthogonally managed. That is, each
 * node has their own respective data buffers. When
 * writing to a node, we need to take into account of the
 * length of the buffer. This value may need to expanded as
 * well as more pages allocated if the amount of bytes to
 * be written exceeds it.
 */
static int
tmpfs_write(struct vnode *vp, struct sio_txn *sio)
{
    struct tmpfs_node *np;
    uint8_t *buf;

    if (sio->buf == NULL || sio->len == 0) {
        return -EINVAL;
    }

    /* This should not happen but you never know */
    if ((np = vp->data) == NULL) {
        return -EIO;
    }

    /* Is this even a regular file? */
    if (np->type != VREG) {
        return -EISDIR;
    }

    spinlock_acquire(&np->lock);

    /*
     * If the residual byte count is zero, we need to
     * allocate a new page to be used. However if this
     * fails we'll throw back an -ENOMEM.
     */
    if (np->len == 0) {
        np->data = dynalloc(TMPFS_BSIZE);
        if (np->data == NULL) {
            spinlock_release(&np->lock);
            return -ENOMEM;
        }
        np->len += TMPFS_BSIZE;
    }

    /*
     * Bring up the real size if we are writing
     * more bytes.
     */
    if (sio->offset >= np->real_size) {
        np->real_size = sio->offset;
    }

    /*
     * If the length to be written exceeds the residual byte
     * count. We will try to expand the buffer by the page
     * size. However, if this fails, we will split the write
     * into a suitable size that does not overflow what we
     * have left.
     */
    if ((sio->offset + sio->len) > np->len) {
        np->data = dynrealloc(np->data, (sio->offset + sio->len));
        if (np->data == NULL) {
            sio->len = np->len;
        } else {
            np->len = sio->offset + sio->len;
        }
    }

    buf = np->data;
    memcpy(&buf[sio->offset], sio->buf, sio->len);
    spinlock_release(&np->lock);
    return sio->len;
}

/*
 * TMPFS read callback for VFS
 */
static int
tmpfs_read(struct vnode *vp, struct sio_txn *sio)
{
    struct tmpfs_node *np;
    uint8_t *buf;

    if (sio->buf == NULL || sio->len == 0) {
        return -EINVAL;
    }

    /* This should not happen but you never know */
    if ((np = vp->data) == NULL) {
        return -EIO;
    }

    /*
     * The node data is only allocated during writes, if
     * we read this file before a write was ever done to it,
     * np->data will be NULL. We must handle this.
     */
    if (np->data == NULL) {
        return 0;
    }

    /* Is this even a regular file? */
    if (np->type != VREG) {
        return -EISDIR;
    }

    spinlock_acquire(&np->lock);

    if (sio->offset > np->real_size) {
        return -EINVAL;
    }

    buf = np->data;
    memcpy(sio->buf, &buf[sio->offset], sio->len);
    spinlock_release(&np->lock);
    return sio->len;
}

/*
 * TMPFS get attribute callback for VFS
 */
static int
tmpfs_getattr(struct vop_getattr_args *args)
{
    struct vnode *vp;
    struct tmpfs_node *np;
    struct vattr attr;

    if ((vp = args->vp) == NULL) {
        return -EIO;
    }
    if ((np = vp->data) == NULL) {
        return -EIO;
    }

    memset(&attr, VNOVAL, sizeof(attr));
    attr.size = np->real_size;
    attr.mode = np->mode;
    *args->res = attr;
    return 0;
}

static int
tmpfs_reclaim(struct vnode *vp)
{
    struct tmpfs_node *np;

    if ((np = vp->data) == NULL) {
        return 0;
    }

    np->vp = NULL;
    return 0;
}

static int
tmpfs_init(struct fs_info *fip)
{
    struct tmpfs_node *np;
    struct vnode *vp;
    struct mount *mp;
    int error;

    /* Grab ourselves a new vnode for /tmp */
    if ((error = vfs_alloc_vnode(&vp, VDIR)) != 0) {
        return error;
    }

    vp->vops = &g_tmpfs_vops;
    mp = vfs_alloc_mount(vp, fip);
    vfs_name_mount(mp, "tmp");
    TAILQ_INSERT_TAIL(&g_mountlist, mp, mnt_list);

    /* Pre-allocate the first entry */
    if ((np = dynalloc(sizeof(*np))) == NULL) {
        return -ENOMEM;
    }

    TAILQ_INIT(&root);
    memset(np, 0, sizeof(*np));

    memcpy(np->rpath, ROOT_RPATH, sizeof(ROOT_RPATH));
    np->type = TMPFS_DIR;
    TAILQ_INIT(&np->dirents);
    TAILQ_INSERT_TAIL(&root, np, link);
    return 0;
}

const struct vops g_tmpfs_vops = {
    .lookup = tmpfs_lookup,
    .getattr = tmpfs_getattr,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .reclaim = tmpfs_reclaim,
    .create = tmpfs_create,
};

const struct vfsops g_tmpfs_vfsops = {
    .init = tmpfs_init
};

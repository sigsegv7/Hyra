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

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <fs/ctlfs.h>
#include <vm/dynalloc.h>
#include <string.h>

#define CTLFS_MPNAME "ctl"
#define CTLFS_ENTRY_MAG 0x43454E54UL  /* 'CENT' */
#define CTLFS_NODE_MAG  0x43544C4EUL  /* 'CTLN' */

#define pr_trace(fmt, ...) kprintf("ctlfs: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static const struct vops ctlfs_vops;

struct ctlfs_hdr {
    uint32_t magic;
    char *name;
};

/*
 * Control fs entry, represents a control
 * file within a ctlfs node.
 * -- HDR START --
 * @magic: Magic number [MUST BE FIRST] (CTLFS_ENTRY_MAG)
 * @name: Entry name    [MUST BE SECOND]
 * -- HDR END --
 * @parent: Parent (ctlfs_node)
 * @io: Ctlfs operations.
 * @mode: Access flags.
 * @link: TAILQ link.
 */
struct ctlfs_entry {
    uint32_t magic;
    char *name;
    struct ctlfs_node *parent;
    const struct ctlops *io;
    mode_t mode;
    TAILQ_ENTRY(ctlfs_entry) link;
};

/*
 * Control fs node, represents a directory
 * within ctlfs. These directories represent
 * devices, each device directory contains
 * control files.
 *
 *              For example:
 *
 *             /ctl/sd1/bsize  # Block size
 *             /ctl/sd1/health # Health
 *             [et cetera]
 *
 * @magic: Magic number [MUST BE FIRST] (CTLFS_NODE_MAG)
 * @name: Name of node  [MUST BE SECOND]
 * @mode: Access flags.
 * @major: Device major number.
 * @minor: Device major number.
 * @eq: Entries for this ctlfs node.
 */
struct ctlfs_node {
    uint32_t magic;
    char *name;
    mode_t mode;
    TAILQ_HEAD(, ctlfs_entry) eq;
    TAILQ_ENTRY(ctlfs_node) link;
};

static TAILQ_HEAD(, ctlfs_node) nodeq;

/*
 * Look up entries within a ctlfs
 * node by name.
 */
static struct ctlfs_entry *
entry_lookup(struct ctlfs_node *cnp, const char *name)
{
    struct ctlfs_entry *ep;

    TAILQ_FOREACH(ep, &cnp->eq, link) {
        if (strcmp(ep->name, name) == 0) {
            return ep;
        }
    }

    return NULL;
}

/*
 * Lookup a ctlfs entry by name.
 */
static struct ctlfs_node *
node_lookup(const char *name)
{
    struct ctlfs_node *cnp;

    TAILQ_FOREACH(cnp, &nodeq, link) {
        if (strcmp(cnp->name, name) == 0) {
            return cnp;
        }
    }

    return NULL;
}

static int
ctlfs_init(struct fs_info *fip)
{
    struct vnode *vp;
    struct mount *mp;
    int error;

    if ((error = vfs_alloc_vnode(&vp, VDIR)) != 0) {
        pr_error("failed to alloc vnode\n");
        return error;
    }

    vp->vops = &ctlfs_vops;
    if ((mp = vfs_alloc_mount(vp, fip)) == NULL) {
        pr_trace("failed to alloc mountpoint\n");
        return -ENOMEM;
    }

    error = vfs_name_mount(mp, CTLFS_MPNAME);
    if (error != 0) {
        pr_trace("failed to mount @ /%s\n", CTLFS_MPNAME);
        return error;
    }

    TAILQ_INSERT_TAIL(&g_mountlist, mp, mnt_list);
    TAILQ_INIT(&nodeq);
    return 0;
}

static int
ctlfs_lookup(struct vop_lookup_args *args)
{
    int error;
    const char *name = args->name;
    struct vnode *vp, *dirvp;
    struct ctlfs_node *cnp = NULL;
    struct ctlfs_entry *enp = NULL;

    if (name == NULL) {
        return -EINVAL;
    }

    dirvp = args->dirvp;
    if (dirvp == NULL) {
        return -EIO;
    }

    /*
     * If we already have data within this vnode
     * it *might* be a control node but we'll have
     * to verify its magic number...
     */
    if (dirvp->data != NULL) {
        cnp = (struct ctlfs_node *)dirvp->data;
        if (cnp->magic != CTLFS_NODE_MAG) {
            pr_error("bad `cnp' magic (name=%s)\n", name);
            return -EIO;
        }
    }

    /*
     * Handle cases where we are looking up
     * relative to a control node.
     */
    if (cnp != NULL) {
        enp = entry_lookup(cnp, name);
        if (enp == NULL) {
            return -ENOENT;
        }

        /* Create a vnode for this enp */
        error = vfs_alloc_vnode(&vp, VCHR);
        if (error != 0) {
            return error;
        }

        vp->data = (void *)enp;
        vp->vops = &ctlfs_vops;
        *args->vpp = vp;
        return 0;
    }

    /* Does this entry exist? */
    if ((cnp = node_lookup(name)) == NULL) {
        return -ENOENT;
    }

    if ((error = vfs_alloc_vnode(&vp, VDIR)) != 0) {
        return error;
    }

    vp->data = cnp;
    vp->vops = &ctlfs_vops;
    *args->vpp = vp;
    return 0;
}

/*
 * Create a ctlfs node (directory) within the
 * root fs.
 *
 * @name: Node name (e.g., "sd1" for "/ctl/sd1/")
 * @dp: Device related arguments (see ctlfs_dev)
 *      Args used:
 *      - mode (access flags)
 *
 */
int
ctlfs_create_node(const char *name, const struct ctlfs_dev *dp)
{
    struct ctlfs_node *cnp;
    size_t namelen;

    if (name == NULL || dp == NULL) {
        return -EINVAL;
    }

    cnp = dynalloc(sizeof(*cnp));
    if (cnp == NULL) {
        return -ENOMEM;
    }

    namelen = strlen(name);
    cnp->name = dynalloc(namelen + 1);
    if (cnp->name == NULL) {
        dynfree(cnp);
        return -ENOMEM;
    }

    memcpy(cnp->name, name, namelen);
    cnp->name[namelen] = '\0';
    cnp->mode = dp->mode;
    cnp->magic = CTLFS_NODE_MAG;
    TAILQ_INSERT_TAIL(&nodeq, cnp, link);
    TAILQ_INIT(&cnp->eq);
    return 0;
}

/*
 * Create a ctlfs entry within a specific node.
 *
 * @name: Name e.g., "/health" for "/ctl/xxx/health".
 * @dp: Device related arguments (see ctlfs_dev)
 *      Args used:
 *      - devname (name of device)
 *      - mode (access flags)
 *      - ops (operations vector)
 */
int
ctlfs_create_entry(const char *name, const struct ctlfs_dev *dp)
{
    struct ctlfs_entry *enp;
    struct ctlfs_node *parent;
    size_t namelen;

    if (name == NULL || dp == NULL) {
        return -EINVAL;
    }
    if (dp->devname == NULL) {
        return -EINVAL;
    }
    if (dp->ops == NULL) {
        return -EINVAL;
    }

    parent = node_lookup(dp->devname);
    if (parent == NULL) {
        pr_trace("could not find %s\n", dp->devname);
        return -ENOENT;
    }

    enp = dynalloc(sizeof(*enp));
    if (enp == NULL) {
        return -ENOMEM;
    }

    namelen = strlen(name);
    enp->name = dynalloc(namelen + 1);
    if (enp->name == NULL) {
        dynfree(enp);
        return -ENOMEM;
    }

    memcpy(enp->name, name, namelen);
    enp->name[namelen] = '\0';
    enp->io = dp->ops;
    enp->magic = CTLFS_ENTRY_MAG;
    enp->mode = dp->mode;
    enp->parent = parent;
    TAILQ_INSERT_TAIL(&parent->eq, enp, link);
    return 0;
}

/*
 * Read a control file
 *
 * Args passed to driver:
 *   - ctlfs_dev.ctlname
 *   - ctlfs_dev.iop
 *   - ctlfs_dev.mode
 */
static int
ctlfs_read(struct vnode *vp, struct sio_txn *sio)
{
    const struct ctlops *iop;
    struct ctlfs_entry *enp;
    struct ctlfs_dev dev;

    if ((enp = vp->data) == NULL) {
        pr_error("no vnode data for ctlfs entry\n");
        return -EIO;
    }
    if (enp->magic != CTLFS_ENTRY_MAG) {
        pr_error("ctlfs entry has bad magic\n");
        return -EIO;
    }
    if ((iop = enp->io) == NULL) {
        pr_error("no i/o ops for ctlfs entry\n");
        return -EIO;
    }
    if (iop->read == NULL) {
        pr_trace("no read op for ctlfs entry\n");
        return -EIO;
    }

    dev.ctlname = enp->name;
    dev.ops = iop;
    dev.mode = enp->mode;
    return iop->read(&dev, sio);
}

static int
ctlfs_reclaim(struct vnode *vp)
{
    struct ctlfs_hdr *hp;

    if (vp->data == NULL) {
        return 0;
    }

    /* Ensure the magic is correct */
    hp = vp->data;
    switch (hp->magic) {
    case CTLFS_NODE_MAG:
    case CTLFS_ENTRY_MAG:
        dynfree(hp->name);
        break;
    default:
        pr_error("reclaim: bad magic in vp\n");
        break;
    }

    dynfree(vp->data);
    return 0;
}

static const struct vops ctlfs_vops = {
    .lookup = ctlfs_lookup,
    .read = ctlfs_read,
    .getattr = NULL,
    .write = NULL,
    .reclaim = ctlfs_reclaim
};

const struct vfsops g_ctlfs_vfsops = {
    .init = ctlfs_init
};

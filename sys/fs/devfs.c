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
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/mount.h>
#include <sys/device.h>
#include <fs/devfs.h>
#include <vm/dynalloc.h>
#include <string.h>

struct devfs_node {
    char *name;
    uint8_t is_block : 1;
    mode_t mode;
    devmajor_t major;
    dev_t dev;
    TAILQ_ENTRY(devfs_node) link;
};

static TAILQ_HEAD(, devfs_node) devlist;

static inline int
cdevsw_read(void *devsw, dev_t dev, struct sio_txn *sio)
{
    struct cdevsw *cdevsw = devsw;

    return cdevsw->read(dev, sio, 0);
}

static inline int
bdevsw_read(void *devsw, dev_t dev, struct sio_txn *sio)
{
    struct bdevsw *bdevsw = devsw;

    return bdevsw->read(dev, sio, 0);
}

static inline int
cdevsw_write(void *devsw, dev_t dev, struct sio_txn *sio)
{
    struct cdevsw *cdevsw = devsw;

    return cdevsw->write(dev, sio, 0);
}

static inline int
bdevsw_write(void *devsw, dev_t dev, struct sio_txn *sio)
{
    struct bdevsw *bdevsw = devsw;

    return bdevsw->write(dev, sio, 0);
}

/*
 * Get a devfs node by name.
 *
 * @name: Name to lookup.
 */
static struct devfs_node *
devfs_get_node(const char *name)
{
    struct devfs_node *dnp;

    TAILQ_FOREACH(dnp, &devlist, link) {
        if (strcmp(dnp->name, name) == 0) {
            return dnp;
        }
    }

    return NULL;
}

static int
devfs_lookup(struct vop_lookup_args *args)
{
    int vtype, error;
    const char *name = args->name;
    struct devfs_node *dnp;
    struct vnode *vp;

    if (*name == '/')
        ++name;

    /* Make sure it isn't a path */
    for (const char *p = name; *p != '\0'; ++p) {
        if (*p == '/') {
            return -ENOENT;
        }
    }

    if ((dnp = devfs_get_node(name)) == NULL)
        return -ENOENT;

    /* Now, create a vnode */
    vtype = (dnp->is_block) ? VBLK : VCHR;
    if ((error = vfs_alloc_vnode(&vp, vtype)) != 0)
        return error;

    vp->data = dnp;
    vp->vops = &g_devfs_vops;
    *args->vpp = vp;
    return 0;
}

static int
devfs_getattr(struct vop_getattr_args *args)
{
    struct vnode *vp;
    struct vattr *attr;
    struct devfs_node *dnp;

    vp = args->vp;
    if ((dnp = vp->data) == NULL) {
        return -EIO;
    }
    if ((attr = args->res) == NULL) {
        return -EIO;
    }

    /*
     * Set stat attributes from device node structure
     * found within vnode data.
     *
     * XXX: Device files have no fixed size, hence why
     *      size is hardwired to 0.
     */
    attr->mode = dnp->mode;
    attr->size = 0;
    return 0;
}

static int
devfs_reclaim(struct vnode *vp)
{
    struct devfs_node *dnp;

    if ((dnp = vp->data) != NULL) {
        dynfree(dnp->name);
        dynfree(vp->data);
    }

    vp->data = NULL;
    return 0;
}

static int
devfs_read(struct vnode *vp, struct sio_txn *sio)
{
    struct devfs_node *dnp;
    void *devsw;

    if ((dnp = vp->data) == NULL)
        return -EIO;

    devsw = dev_get(dnp->major, dnp->dev);

    if (!dnp->is_block)
        return cdevsw_read(devsw, dnp->dev, sio);

    /* Block device */
    return bdevsw_read(devsw, dnp->dev, sio);
}

static int
devfs_write(struct vnode *vp, struct sio_txn *sio)
{
    struct devfs_node *dnp;
    void *devsw;

    if ((dnp = vp->data) == NULL)
        return -EIO;

    devsw = dev_get(dnp->major, dnp->dev);

    if (!dnp->is_block) {
        return cdevsw_write(devsw, dnp->dev, sio);
    }

    /* Block device */
    return bdevsw_write(devsw, dnp->dev, sio);
}

static int
devfs_init(struct fs_info *fip)
{
    struct vnode *vp;
    struct mount *mp;
    int error;

    /* Create a new vnode for devfs */
    if ((error = vfs_alloc_vnode(&vp, VDIR)) != 0)
        return error;

    vp->vops = &g_devfs_vops;
    TAILQ_INIT(&devlist);
    mp = vfs_alloc_mount(vp, fip);

    vfs_name_mount(mp, "dev");
    TAILQ_INSERT_TAIL(&g_mountlist, mp, mnt_list);
    return 0;
}

/*
 * Create an entry within devfs.
 *
 * @name: Device name.
 * @major: Device major.
 * @dev: Device minor.
 * @mode: Permissions mask
 */
int
devfs_create_entry(const char *name, devmajor_t major, dev_t dev, mode_t mode)
{
    struct devfs_node *dnp;
    size_t name_len;

    dnp = dynalloc(sizeof(*dnp));
    if (dnp == NULL)
        return -ENOMEM;

    name_len = strlen(name);
    dnp->name = dynalloc(sizeof(char) * name_len + 1);
    if (dnp->name == NULL) {
        dynfree(dnp);
        return -ENOMEM;
    }

    memcpy(dnp->name, name, name_len);
    dnp->name[name_len] = '\0';

    dnp->major = major;
    dnp->dev = dev;
    dnp->mode = mode;
    TAILQ_INSERT_TAIL(&devlist, dnp, link);
    return 0;
}

const struct vops g_devfs_vops = {
    .lookup = devfs_lookup,
    .reclaim = devfs_reclaim,
    .read = devfs_read,
    .write = devfs_write,
    .getattr = devfs_getattr
};

const struct vfsops g_devfs_vfsops = {
    .init = devfs_init
};

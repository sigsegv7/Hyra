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

#include <fs/devfs.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <sys/spinlock.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <vm/dynalloc.h>
#include <string.h>

struct device_node {
    struct spinlock lock;
    char *name;
    uint8_t is_block : 1;
    dev_t major, minor;
    TAILQ_ENTRY(device_node) link;
};

static TAILQ_HEAD(, device_node) nodes;
static bool nodelist_init = false;

static struct device_node *
node_from_name(const char *name)
{
    struct device_node *n;

    TAILQ_FOREACH(n, &nodes, link) {
        if (strcmp(n->name, name) == 0) {
            return n;
        }
    }

    return NULL;
}

static int
blkdev_read(struct device *dev, struct device_node *node, struct sio_txn *sio)
{
    char *buf;
    struct sio_txn dev_txn = {0};
    size_t buf_size = dev->blocksize * sio->len;

    if (dev->blocksize == 0 || sio->len == 0) {
        /* Sizes can't be zero! */
        return -EIO;
    }

    spinlock_acquire(&node->lock);
    buf = dynalloc_memalign(buf_size, 0x1000);

    if (buf == NULL) {
        spinlock_release(&node->lock);
        return -ENOMEM;
    }

    dev_txn.len = __DIV_ROUNDUP(sio->len, dev->blocksize);
    dev_txn.buf = buf;
    dev_txn.offset = sio->offset;
    dev->read(dev, &dev_txn);
    spinlock_release(&node->lock);

    for (size_t i = 0; i < sio->len; ++i) {
        ((uint8_t *)sio->buf)[i] = buf[i];
    }

    dynfree(buf);
    return sio->len;
}


static int
vop_vget(struct vnode *parent, const char *name, struct vnode **vp)
{
    struct device_node *dev;
    struct vnode *vnode;
    int status, vtype;

    if (!nodelist_init) {
        return -EIO;
    }

    if ((dev = node_from_name(name)) == NULL) {
        return -ENOENT;
    }

    vtype = dev->is_block ? VBLK : VCHR;
    if ((status = vfs_alloc_vnode(&vnode, NULL, vtype)) != 0) {
        return status;
    }

    vnode->parent = parent;
    vnode->data = dev;
    vnode->vops = &g_devfs_vops;
    *vp = vnode;
    return 0;
}

static int
vop_read(struct vnode *vp, struct sio_txn *sio)
{
    struct device_node *node;
    struct device *dev;

    if (vp == NULL) {
        return -EIO;
    }

    node = vp->data;
    dev = device_fetch(node->major, node->minor);
    return blkdev_read(dev, node, sio);
}

static int
devfs_init(struct fs_info *info)
{
    TAILQ_INIT(&nodes);
    nodelist_init = true;
    return 0;
}

static int
devfs_make_devicenode(const char *name, struct device_node **node_out)
{
    size_t name_len = 0;
    const char *p = name;
    struct device_node *node;

    /*
     * Only one filename, no paths.
     *
     * TODO: Do something better here...
     */
    for (; *p; ++p, ++name_len) {
        if (*p == '/')
            return -EINVAL;
    }

    /* Ensure this filename has valid chars */
    if (!vfs_is_valid_path(name)) {
        return -EINVAL;
    }

    node = dynalloc(sizeof(struct device_node));
    if (node == NULL)
        return -ENOMEM;

    node->name = dynalloc(sizeof(char) * name_len);
    if (node->name == NULL)
        return -ENOMEM;

    memcpy(node->name, name, name_len + 1);
    *node_out = node;
    return 0;
}

int
devfs_add_blkdev(const char *name, const struct device *blkdev)
{
    struct device_node *node;
    int status;

    if ((status = devfs_make_devicenode(name, &node)) != 0) {
        return status;
    }

    node->major = blkdev->major;
    node->minor = blkdev->minor;
    node->is_block = 1;
    TAILQ_INSERT_HEAD(&nodes, node, link);
    return 0;
}

struct vfsops g_devfs_ops = {
    .init = devfs_init
};

struct vops g_devfs_vops = {
    .vget = vop_vget,
    .read = vop_read
};

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

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/errno.h>
#include <vm/dynalloc.h>
#include <sys/vfs.h>
#include <fs/procfs.h>
#include <string.h>

struct proc_node {
    struct spinlock lock;
    struct proc_entry *entry;
    char *name;
    TAILQ_ENTRY(proc_node) link;
};

static TAILQ_HEAD(, proc_node) proc_nodes;
static bool nodelist_init = false;

static struct proc_node *
name_to_node(const char *name)
{
    struct proc_node *n;

    TAILQ_FOREACH(n, &proc_nodes, link) {
        if (strcmp(n->name, name) == 0) {
            return n;
        }
    }

    return NULL;
}

static int
procfs_make_node(const char *name, struct proc_node **res)
{
    size_t name_len = 0;
    const char *p = name;
    struct proc_node *n;

    /* Disallow paths */
    for (; *p; ++p, ++name_len) {
        if (*p == '/') {
            return -EINVAL;
        }
    }

    if (!vfs_is_valid_path(name))
        return -EINVAL;

    n = dynalloc(sizeof(struct proc_node));
    if (n == NULL)
        return -ENOMEM;

    n->name = dynalloc(sizeof(char) * name_len);
    if (n->name == NULL)
        return -ENOMEM;

    memcpy(n->name, name, name_len + 1);
    *res = n;
    return 0;
}

struct proc_entry *
procfs_alloc_entry(void)
{
    struct proc_entry *entry;

    entry = dynalloc(sizeof(*entry));
    if (entry == NULL)
        return NULL;

    memset(entry, 0, sizeof(*entry));
    return entry;
}

int
procfs_add_entry(const char *name, struct proc_entry *entry)
{
    struct proc_node *proc;
    int status;

    if (name == NULL || entry == NULL)
        return -EINVAL;
    if ((status = procfs_make_node(name, &proc)) != 0)
        return status;

    proc->entry = entry;
    TAILQ_INSERT_HEAD(&proc_nodes, proc, link);
    return 0;
}

static int
procfs_init(struct fs_info *info, struct vnode *source)
{
    if (source != NULL)
        return -EINVAL;


    TAILQ_INIT(&proc_nodes);
    nodelist_init = true;
    procfs_populate();
    return 0;
}

static int
procfs_rw_vnode(struct vnode *vp, struct sio_txn *sio, bool write)
{
    struct proc_node *proc;
    struct proc_entry *entry;

    if (vp == NULL)
        return -EIO;

    proc = vp->data;
    entry = proc->entry;

    return write ? entry->write(entry, sio)
                 : entry->read(entry, sio);
}

static int
vop_write(struct vnode *vp, struct sio_txn *sio)
{
    return procfs_rw_vnode(vp, sio, true);
}

static int
vop_read(struct vnode *vp, struct sio_txn *sio)
{
    return procfs_rw_vnode(vp, sio, false);
}

static int
vop_open(struct vnode *vp)
{
    return 0;
}

static int
vop_close(struct vnode *vp)
{
    return 0;
}

static int
vop_vget(struct vnode *parent, const char *name, struct vnode **vp)
{
    struct proc_node *proc;
    struct vnode *vnode;
    int status;

    if (!nodelist_init)
        return -EIO;
    if ((proc = name_to_node(name)) == NULL)
        return -ENOENT;
    if ((status = vfs_alloc_vnode(&vnode, NULL, VREG)) != 0)
        return status;

    vnode->parent = parent;
    vnode->data = proc;
    vnode->vops = &g_procfs_vops;
    *vp = vnode;
    return 0;
}

struct vfsops g_procfs_ops = {
    .init = procfs_init
};

struct vops g_procfs_vops = {
    .vget = vop_vget,
    .read = vop_read,
    .write = vop_write,
    .open = vop_open,
    .close = vop_close
};

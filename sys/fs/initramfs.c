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
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/limine.h>
#include <sys/panic.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <fs/initramfs.h>
#include <vm/dynalloc.h>
#include <string.h>

#define OMAR_EOF "RAMO"
#define OMAR_REG    0
#define OMAR_DIR    1
#define BLOCK_SIZE 512

/*
 * File or directory.
 */
struct initramfs_node {
    const char *path;       /* Path */
    void *data;             /* File data */
    size_t size;            /* File size */
    mode_t mode;            /* Perms and type */
};

/*
 * The OMAR file header, describes the basics
 * of a file.
 *
 * @magic: Header magic ("OMAR")
 * @len: Length of the file
 * @namelen: Length of the filename
 */
struct __packed omar_hdr {
    char magic[4];
    uint8_t type;
    uint8_t namelen;
    uint32_t len;
};

static volatile struct limine_module_request mod_req = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static const char *initramfs = NULL;
static uint64_t initramfs_size;

/*
 * Fetch a module from the bootloader.
 * This is used to fetch the ramfs image.
 */
static char *
get_module(const char *path, uint64_t *size) {
    for (uint64_t i = 0; i < mod_req.response->module_count; ++i) {
        if (strcmp(mod_req.response->modules[i]->path, path) == 0) {
            *size = mod_req.response->modules[i]->size;
            return mod_req.response->modules[i]->address;
        }
    }

    return NULL;
}

/*
 * Get a file from initramfs
 *
 * @path: Path of file to get.
 * @res: Pointer to new resulting node.
 */
static int
initramfs_get_file(const char *path, struct initramfs_node *res)
{
    struct initramfs_node node;
    const struct omar_hdr *hdr;
    const char *p, *name;
    char namebuf[256];
    off_t off;

    p = initramfs;
    for (;;) {
        hdr = (struct omar_hdr *)p;
        if (strncmp(hdr->magic, OMAR_EOF, sizeof(OMAR_EOF)) == 0) {
            break;
        }

        /* Ensure the file is valid */
        if (strncmp(hdr->magic, "OMAR", 4) != 0) {
            /* Bad magic */
            return -EINVAL;
        }
        if (hdr->namelen > sizeof(namebuf) - 1) {
            return -EINVAL;
        }

        name = (char *)p + sizeof(struct omar_hdr);
        memcpy(namebuf, name, hdr->namelen);
        namebuf[hdr->namelen] = '\0';

        /* Compute offset to next block */
        if (hdr->type == OMAR_DIR) {
            off = 512;
        } else {
            off = ALIGN_UP(sizeof(*hdr) + hdr->namelen + hdr->len, BLOCK_SIZE);
        }

        /* Skip header and name, right to the data */
        p = (char *)hdr + sizeof(struct omar_hdr);
        p += hdr->namelen;

        if (strcmp(namebuf, path) == 0) {
            node.mode = 0700;
            node.size = hdr->len;
            node.data = (void *)p;
            *res = node;
            return 0;
        }

        hdr = (struct omar_hdr *)((char *)hdr + off);
        p = (char *)hdr;
        memset(namebuf, 0, sizeof(namebuf));
    }

    return -ENOENT;
}

static int
initramfs_lookup(struct vop_lookup_args *args)
{
    int status, vtype;
    struct initramfs_node *n;
    struct vnode *vp;
    const char *path = args->name;

    if (*path == '/') {
        ++path;
    }

    n = dynalloc(sizeof(*n));
    if (n == NULL) {
        return -ENOMEM;
    }

    /* Now does this file exist? */
    if ((status = initramfs_get_file(path, n)) != 0) {
        dynfree(n);
        return status;
    }

    vtype = ISSET(n->mode, 0040000) ? VDIR : VREG;

    /* Try to create a new vnode */
    if ((status = vfs_alloc_vnode(&vp, vtype)) != 0) {
        dynfree(n);
        return status;
    }

    vp->data = n;
    vp->vops = &g_initramfs_vops;
    *args->vpp = vp;
    return 0;
}

static int
initramfs_getattr(struct vop_getattr_args *args)
{
    struct vnode *vp;
    struct initramfs_node *n;
    struct vattr attr;

    if ((vp = args->vp) == NULL)
        return -EIO;
    if ((n = vp->data) == NULL)
        return -EIO;

    memset(&attr, VNOVAL, sizeof(attr));
    attr.mode = n->mode;
    attr.size = n->size;
    *args->res = attr;
    return 0;
}

static int
initramfs_read(struct vnode *vp, struct sio_txn *sio)
{
    struct initramfs_node *n = vp->data;
    uint8_t *src, *dest;
    uint32_t count = 0;

    /* Ensure pointers are valid */
    if (n == NULL)
        return -EIO;
    if (sio->buf == NULL)
        return -EIO;
    if (sio->len > n->size)
        sio->len = n->size;

    src = n->data;
    dest = sio->buf;

    /* Copy the file data */
    for (size_t i = 0; i < sio->len; ++i) {
        if ((sio->offset + i) >= n->size) {
            break;
        }
        dest[i] = src[sio->offset + i];
        ++count;
    }

    return count;
}

static int
initramfs_reclaim(struct vnode *vp)
{
    if (vp->data != NULL) {
        dynfree(vp->data);
    }

    vp->data = NULL;
    return 0;
}

static int
initramfs_init(struct fs_info *fip)
{
    struct mount *mp;
    int status;

    initramfs = get_module("/boot/ramfs.omar", &initramfs_size);
    if (initramfs == NULL) {
        panic("failed to open initramfs OMAR image\n");
    }

    status = vfs_alloc_vnode(&g_root_vnode, VDIR);
    if (__unlikely(status != 0)) {
        panic("failed to create root vnode for ramfs\n");
    }

    g_root_vnode->vops = &g_initramfs_vops;
    mp = vfs_alloc_mount(g_root_vnode, fip);
    TAILQ_INSERT_TAIL(&g_mountlist, mp, mnt_list);
    return 0;
}

const struct vops g_initramfs_vops = {
    .lookup = initramfs_lookup,
    .read = initramfs_read,
    .write = NULL,
    .reclaim = initramfs_reclaim,
    .getattr = initramfs_getattr,
    .create = NULL,
};

const struct vfsops g_initramfs_vfsops = {
    .init = initramfs_init
};

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

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/limine.h>
#include <sys/panic.h>
#include <sys/vnode.h>
#include <fs/initramfs.h>
#include <vm/dynalloc.h>
#include <string.h>

#define CPIO_TRAILER "TRAILER!!!"

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
 * ODC CPIO header
 */
struct cpio_hdr {
    char c_magic[6];
    char c_dev[6];
    char c_ino[6];
    char c_mode[6];
    char c_uid[6];
    char c_gid[6];
    char c_nlink[6];
    char c_rdev[6];
    char c_mtime[11];
    char c_namesize[6];
    char c_filesize[11];
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
 * Convert octal to base 10
 */
static uint32_t
oct2dec(const char *in, size_t sz)
{
    size_t val = 0;

    for (size_t i = 0; i < sz; ++i) {
        val = val * 8 + (in[i] - '0');
    }

    return val;
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
    const struct cpio_hdr *hdr;
    struct initramfs_node node;
    uintptr_t addr;
    size_t namesize, filesize;
    mode_t mode;

    addr = (uintptr_t)initramfs;
    for (;;) {
        hdr = (void *)addr;
        namesize = oct2dec(hdr->c_namesize, sizeof(hdr->c_namesize));
        filesize = oct2dec(hdr->c_filesize, sizeof(hdr->c_filesize));
        mode = oct2dec(hdr->c_mode, sizeof(hdr->c_mode));

        /* Make sure the magic is correct */
        if (strncmp(hdr->c_magic, "070707", 6) != 0) {
            return -EINVAL;
        }

        addr += sizeof(struct cpio_hdr);
        node.path = (const char *)addr;

        /* Is this the requested file? */
        if (strcmp(node.path, path) == 0) {
            node.data = (void *)(addr + namesize);
            node.size = filesize;
            node.mode = mode;
            *res = node;
            return 0;
        }

        /* Get next header and see if we are at the end */
        addr += (namesize + filesize);
        if (strcmp(node.path, CPIO_TRAILER) == 0) {
            break;
        }
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

    return 0;
}

static int
initramfs_init(struct fs_info *fip)
{
    struct mount *mp;
    int status;

    initramfs = get_module("/boot/ramfs.cpio", &initramfs_size);
    if (initramfs == NULL) {
        panic("Failed to open initramfs cpio image\n");
    }

    status = vfs_alloc_vnode(&g_root_vnode, VDIR);
    if (__unlikely(status != 0)) {
        panic("Failed to create root vnode for ramfs\n");
    }

    g_root_vnode->vops = &g_initramfs_vops;
    mp = vfs_alloc_mount(g_root_vnode, fip);
    TAILQ_INSERT_TAIL(&g_mountlist, mp, mnt_list);
    return 0;
}

const struct vops g_initramfs_vops = {
    .lookup = initramfs_lookup,
    .read = initramfs_read,
    .reclaim = initramfs_reclaim
};

const struct vfsops g_initramfs_vfsops = {
    .init = initramfs_init
};

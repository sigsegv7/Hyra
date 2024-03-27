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

#include <fs/initramfs.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/limine.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <vm/dynalloc.h>
#include <string.h>

__MODULE_NAME("initramfs");
__KERNEL_META("$Hyra$: initramfs.c, Ian Marco Moffett, "
              "Initial ram filesystem");

static volatile struct limine_module_request mod_req = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static const char *initramfs = NULL;
static size_t initramfs_size = 0;

#define TAR_TYPEFLAG_NORMAL     '0'
#define TAR_TYPEFLAG_HARDLINK   '1'
#define TAR_TYPEFLAG_DIR        '5'

struct tar_hdr {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char link_name[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char dev_major[8];
    char dev_minor[8];
    char prefix[155];
};

static struct tar_hdr *initramfs_from_path(struct tar_hdr *hdr,
                                           const char *path);

static size_t
getsize(const char *in);

static inline char *
hdr_to_contents(struct tar_hdr *hdr)
{
    return ((char*)hdr) + 0x200;
}

static int
vop_vget(struct vnode *parent, const char *name, struct vnode **vp)
{
    struct tar_hdr *hdr;
    struct vnode *vnode;
    int status;
    int vtype = VREG;

    if (initramfs == NULL) {
        return -EIO;
    }

    hdr = initramfs_from_path((void *)initramfs, name);

    if (hdr == NULL) {
        return -ENOENT;
    }

    if (hdr->type == TAR_TYPEFLAG_DIR) {
        vtype = VDIR;
    }

    /* Allocate vnode for this file */
    if ((status = vfs_alloc_vnode(&vnode, NULL, vtype)) != 0) {
        return status;
    }

    vnode->parent = parent;
    vnode->data = hdr;

    vnode->vops = &g_initramfs_vops;
    *vp = vnode;
    return 0;
}

static int
vop_read(struct vnode *vp, struct sio_txn *sio)
{
    struct tar_hdr *hdr;
    size_t size;
    char *contents;
    char *buf = sio->buf;

    if (vp->data == NULL) {
        return -EIO;
    }

    hdr = vp->data;
    size = getsize(hdr->size);
    contents = hdr_to_contents(hdr);

    for (size_t i = sio->offset; i < sio->len; ++i) {
        if (i >= size) {
            return i + 1;
        }
        buf[i - sio->offset] = contents[i];
    }

    return sio->len;
}

static int
vop_getattr(struct vnode *vp, struct vattr *vattr)
{
    struct tar_hdr *hdr = vp->data;

    if (hdr == NULL) {
        return -EIO;
    }

    switch (hdr->type) {
    case TAR_TYPEFLAG_NORMAL:
        vattr->type = VREG;
        break;
    case TAR_TYPEFLAG_DIR:
        vattr->type = VDIR;
        break;
    }

    vattr->size = getsize(hdr->size);
    return 0;
}

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

static size_t
getsize(const char *in)
{
    size_t size = 0, count = 1;

    for (size_t j = 11; j > 0; --j, count *= 8) {
        size += (in[j-1]-'0')*count;
    }

    return size;
}

static int
initramfs_init(struct fs_info *info)
{
    initramfs = get_module("/boot/initramfs.tar", &initramfs_size);
    info->caps = FSCAP_FULLPATH;

    if (initramfs == NULL) {
        panic("Failed to load initramfs\n");
    }

    return 0;
}

static struct tar_hdr *
initramfs_from_path(struct tar_hdr *hdr, const char *path)
{

    uintptr_t addr = (uintptr_t)hdr;
    size_t size;

    if (*path != '/') {
        return NULL;
    }
    ++path;

    while (strcmp(hdr->magic, "ustar") == 0) {
        size = getsize(hdr->size);

        if (strcmp(hdr->filename, path) == 0) {
            return hdr;
        }

        addr += 512 + __ALIGN_UP(size, 512);
        hdr = (struct tar_hdr *)addr;
    }

    return NULL;
}

const char *
initramfs_open(const char *path)
{
    struct tar_hdr *hdr;

    if (initramfs == NULL) {
        return NULL;
    }

    if (strlen(path) > 99) {
        return NULL;
    }

    hdr = initramfs_from_path((void *)initramfs, path);
    return (hdr == NULL) ? NULL : hdr_to_contents(hdr);
}

struct vfsops g_initramfs_ops = {
    .init = initramfs_init,
};

struct vops g_initramfs_vops = {
    .vget = vop_vget,
    .read = vop_read,
    .getattr = vop_getattr
};

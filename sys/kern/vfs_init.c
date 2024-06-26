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

#include <sys/vfs.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <fs/initramfs.h>
#include <fs/devfs.h>
#include <fs/procfs.h>
#include <assert.h>
#include <string.h>

__MODULE_NAME("vfs");
__KERNEL_META("$Hyra$: vfs.c, Ian Marco Moffett, "
              "Hyra Virtual File System");

#define INITRAMFS_ID 0
#define DEVFS_ID 1
#define PROCFS_ID 2

static struct fs_info filesystems[] = {
    [INITRAMFS_ID] = { "initramfs", &g_initramfs_ops, NULL},
    [DEVFS_ID]     = { "dev", &g_devfs_ops, &g_devfs_vops },
    [PROCFS_ID]    = { "proc", &g_procfs_ops, &g_procfs_vops }
};

struct vnode *g_root_vnode = NULL;

struct fs_info *
vfs_byname(const char *name)
{
    for (int i = 0; i < __ARRAY_COUNT(filesystems); ++i) {
        if (strcmp(filesystems[i].name, name) == 0) {
            return &filesystems[i];
        }
    }

    return NULL;
}

void
vfs_init(void)
{
    struct fs_info *info;
    struct vfsops *vfsops;

    vfs_mount_init();
    __assert(vfs_alloc_vnode(&g_root_vnode, NULL, VDIR) == 0);

    for (int i = 0; i < __ARRAY_COUNT(filesystems); ++i) {
        info = &filesystems[i];
        vfsops = info->vfsops;

        __assert(vfsops->init != NULL);
        __assert(vfs_mount(info->name, 0, info) == 0);
        vfsops->init(info, NULL);
    }

    g_root_vnode->vops = &g_initramfs_vops;
    g_root_vnode->fs = &filesystems[INITRAMFS_ID];
}

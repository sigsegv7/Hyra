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

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/panic.h>
#include <string.h>

struct vnode *g_root_vnode = NULL;
static struct fs_info fs_list[] = {
    {MOUNT_RAMFS, &g_initramfs_vfsops, 0, 0},
    {MOUNT_DEVFS, &g_devfs_vfsops, 0, 0},
    {MOUNT_CTLFS, &g_ctlfs_vfsops, 0, 0}
};

void
vfs_init(void)
{
    struct fs_info *fs;
    const struct vfsops *vfsops;

    TAILQ_INIT(&g_mountlist);

    for (size_t i= 0; i < NELEM(fs_list); ++i) {
        fs = &fs_list[i];
        vfsops = fs->vfsops;

        /* Try to initialize the filesystem */
        if (vfsops->init != NULL) {
            vfsops->init(fs);
        }
    }

    /* Use global vcache by default */
    vfs_vcache_migrate(VCACHE_TYPE_GLOBAL);
}

struct fs_info *
vfs_byname(const char *name)
{
    for (int i = 0; i < NELEM(fs_list); ++i) {
        if (strcmp(fs_list[i].name, name) == 0) {
            return &fs_list[i];
        }
    }

    return NULL;
}

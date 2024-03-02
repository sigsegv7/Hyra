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

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/vnode.h>

#define FS_NAME_MAX 16 /* Max length of FS type name including nul */

struct fs_info;
struct mount;

struct vfsops {
    int(*init)(struct fs_info *info);
};

struct mount {
    int flags;
    size_t phash;               /* Path hash */
    struct vnode *vnode;
    TAILQ_ENTRY(mount) link;
};

struct fs_info {
    char name[FS_NAME_MAX];     /* Filesystem type name */
    struct vfsops *vfsops;      /* Filesystem operations */
    struct mount *mp_root;
};

/*
 * Mount flags
 */
#define MNT_RDONLY  0x00000001

#if defined(_KERNEL)
int vfs_mount(const char *path, int mntflags);
int vfs_get_mp(const char *path, struct mount **mp);
void vfs_mount_init(void);
#endif  /* defined(_KERNEL) */

#endif  /* !_SYS_MOUNT_H_ */

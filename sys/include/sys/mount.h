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
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/spinlock.h>

#if defined(_KERNEL)

#define FS_NAME_MAX 16  /* Length of fs name including nul */
#define NAME_MAX 256    /* Max name of filename (not including nul) */

/*
 * Filesystem types.
 */
#define MOUNT_RAMFS "initramfs"

struct vfsops;
struct mount;

/* Mount list */
typedef TAILQ_HEAD(, mount) mountlist_t;
extern mountlist_t g_mountlist;

/* Filesystem operations */
extern const struct vfsops g_initramfs_vfsops;

struct mount {
    char *name;
    struct spinlock lock;
    struct vnode *vp;
    const struct vfsops *mnt_ops;
    void *data;
    TAILQ_ENTRY(mount) mnt_list;
};

struct fs_info {
    char name[FS_NAME_MAX];         /* FS Type name */
    const struct vfsops *vfsops;    /* Operations vector */
    int flags;                      /* Flags for this filesystem */
    int refcount;                   /* Mount count of this type */
};

struct vfsops {
    int(*init)(struct fs_info *fip);
    int(*mount)(struct mount *mp, const char *path, void *data,
        struct nameidata *ndp);
};

void vfs_init(void);
int vfs_name_mount(struct mount *mp, const char *name);

struct mount *vfs_alloc_mount(struct vnode *vp, struct fs_info *fip);
struct fs_info *vfs_byname(const char *name);

#endif  /* _KERNEL */
#endif  /* _SYS_MOUNT_H_ */

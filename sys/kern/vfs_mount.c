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
#include <sys/queue.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <sys/errno.h>
#include <vm/dynalloc.h>
#include <assert.h>

/* TODO: Make this more flexible */
#define MOUNTLIST_SIZE 8

/* Mountlist entry */
struct mountlist_entry {
    TAILQ_HEAD(, mount) buckets;
};

static struct mountlist_entry *mountlist = NULL;

static int
vfs_create_mp(const char *path, int mntflags, struct mount **mp_out)
{
    struct mount *mp;

    mp = dynalloc(sizeof(struct mount));
    if (mp == NULL) {
        return -ENOMEM;
    }

    mp->flags = mntflags;
    *mp_out = mp;
    return 0;
}

/*
 * Mount a mountpoint
 *
 * @path: Path this mountpoint belongs to
 * @mntflags: Flags to mount with
 * @fs: Filesystem to mount
 *
 * If this mount entry already exists, -EEXIST
 * will be returned.
 */
int
vfs_mount(const char *path, int mntflags, struct fs_info *fs)
{
    size_t hash;
    int status;
    struct mountlist_entry *entry;
    struct mount *mp;

    /* Exclude leading slash */
    if (*path == '/') {
        ++path;
    }

    hash = vfs_hash_path(path);

    if ((status = vfs_create_mp(path, mntflags, &mp)) != 0) {
        return status;
    }

    if (hash == -1) {
        /* Something is wrong with the path */
        return -EINVAL;
    }

    if (vfs_get_mp(path, NULL) == 0) {
        /* mount hit, do not duplicate this entry */
        return -EEXIST;
    }

    if ((status = vfs_alloc_vnode(&fs->vnode, mp, VDIR)) != 0) {
        return status;
    }

    mp->phash = hash;
    mp->fs = fs;
    fs->vnode->vops = fs->vops;

    entry = &mountlist[hash % MOUNTLIST_SIZE];
    TAILQ_INSERT_TAIL(&entry->buckets, mp, link);
    return 0;
}

/*
 * Fetch mountpoint
 *
 * @path: Path of target mountpoint
 * @mp: Pointer of variable where mountpoint fetched will
 *      be stored
 *
 * Returns 0 upon a mplist hit, on a mplist miss this
 * function returns -ENOENT.
 */
int
vfs_get_mp(const char *path, struct mount **mp)
{
    size_t hash;
    struct mountlist_entry *entry;
    struct mount *mount_iter;

    /* Exclude leading slash */
    if (*path == '/') {
        ++path;
    }

    hash = vfs_hash_path(path);

    if (hash == 0) {
        /* Something is wrong with the path */
        return -EINVAL;
    }

    entry = &mountlist[hash % MOUNTLIST_SIZE];
    TAILQ_FOREACH(mount_iter, &entry->buckets, link) {
        if (mount_iter->phash == hash) {
            if (mp != NULL) *mp = mount_iter;
            return 0;
        }
    }

    return -ENOENT;
}

void
vfs_mount_init(void)
{
    mountlist = dynalloc(sizeof(struct mountlist_entry) * MOUNTLIST_SIZE);
    __assert(mountlist != NULL);

    for (size_t i = 0; i < MOUNTLIST_SIZE; ++i) {
        TAILQ_INIT(&mountlist[i].buckets);
    }
}

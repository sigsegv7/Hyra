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

#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/errno.h>
#include <vm/dynalloc.h>
#include <string.h>

/*
 * Fetches the filename within a path at
 * the nth index denoted by `idx'
 *
 * Returns memory allocated by dynalloc()
 * containing the filename.
 *
 * XXX: MUST FREE RETURN VALUE WITH dynfree() WHEN
 *      DONE!
 */
static char *
vfs_get_fname_at(const char *path, size_t idx)
{
    size_t pathlen = strlen(path);
    size_t fname_len;

    char *path_tmp = dynalloc(pathlen + 2);
    char *ret = NULL;
    char *start_ptr, *ptr;

    /* Make one-based */
    ++idx;

    if (path_tmp == NULL) {
        return NULL;
    }

    ptr = path_tmp;
    memcpy(path_tmp, path, pathlen + 1);

    /*
     * We want to by default have a '/' at the end
     * to keep the parsing logic from getting more
     * complicated than it needs to be.
     */
    path_tmp[pathlen] = '/';
    path_tmp[pathlen + 1] = '\0';

    /* Skip any leading slashes */
    while (*ptr == '/')
        ++ptr;

    start_ptr = ptr;

    /* Get each filename */
    while (*ptr != '\0') {
        /* Handle duplicate delimiter */
        if (*ptr == '/' && *(ptr + 1) == '/') {
            /*
             * Snip this delimiter and skip, the next
             * will be read and filename returned (if of course
             * the index is reached).
             */
            *(ptr++) = '\0';
            continue;
        }

        if (*ptr == '/') {
            *(ptr++) = '\0';

            /* Continue if index not reached */
            if ((--idx) != 0) {
                start_ptr = ptr;
                continue;
            }

            /* Index has been reached, start_ptr contains name */
            fname_len = strlen(start_ptr);
            ret = dynalloc(fname_len + 1);

            if (ret != NULL) {
                memcpy(ret, start_ptr, fname_len + 1);
            }
            break;
        }

        ++ptr;
    }

    dynfree(path_tmp);
    return ret;
}

/*
 * Search for a path within a mountpoint.
 *
 * @mp: Mountpoint to search in.
 * @path: Path to search for.
 */
static struct vnode *
namei_mp_search(struct mount *mp, const char *path)
{
    struct vop_lookup_args lookup_args;
    struct vnode *vp = mp->vp;
    char *name;
    int status;

    for (size_t i = 1;; ++i) {
        name = vfs_get_fname_at(path, i);
        if (name == NULL)
            break;

        lookup_args.name = name;
        lookup_args.dirvp = vp;
        lookup_args.vpp = &vp;

        status = vfs_vop_lookup(vp, &lookup_args);
        dynfree(name);

        if (status != 0) {
            return NULL;
        }
    }

    return vp;
}

/*
 * Convert a path to a vnode.
 *
 * @ndp: Nameidata containing the path and resulting
 *       vnode.
 */
int
namei(struct nameidata *ndp)
{
    struct vnode *vp = NULL;
    struct mount *mp;
    struct vop_lookup_args lookup_args;
    const char *path = ndp->path;
    char *name;
    int status;

    if (path == NULL) {
        return -EINVAL;
    }

    /* Path must start with "/" */
    if (*path != '/') {
        return -EINVAL;
    }

    /* Just return the root vnode if we can */
    if (strcmp(path, "/") == 0) {
        ndp->vp = g_root_vnode;
        return 0;
    }

    /*
     * Start looking at the root vnode. If we can't find
     * what we are looking for, we'll try traversing the
     * mountlist.
     *
     * Some filesystems (like initramfs) may only understand
     * full paths, so try passing it through.
     */
    lookup_args.name = path;
    lookup_args.dirvp = g_root_vnode;
    lookup_args.vpp = &vp;
    status = vfs_vop_lookup(lookup_args.dirvp, &lookup_args);

    /* Did we find it in the root */
    if (status == 0) {
        ndp->vp = vp;
        return 0;
    }

    /* Look through the mountlist */
    TAILQ_FOREACH(mp, &g_mountlist, mnt_list) {
        /* If it is unamed, can't do anything */
        if (mp->name == NULL)
            continue;

        lookup_args.dirvp = mp->vp;
        name = vfs_get_fname_at(path, 0);

        /* If the name matches, search within */
        if (strcmp(mp->name, name) == 0)
            vp = namei_mp_search(mp, path);

        /* Did we find it at this mountpoint? */
        if (vp != NULL) {
            ndp->vp = vp;
            return 0;
        }

        dynfree(name);
    }

    return -ENOENT;
}

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
#include <sys/vfs.h>
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
char *
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
 * Fetches a vnode from a path.
 *
 * @path: Path to fetch vnode from.
 * @vp: Output var for fetched vnode.
 *
 * Returns 0 on success.
 */
int
vfs_path_to_node(const char *path, struct vnode **vp)
{
    struct vnode *vnode = g_root_vnode;
    struct fs_info *fs;
    char *name;
    int s = 0, fs_caps = 0;

    if (strcmp(path, "/") == 0 || !vfs_is_valid_path(path)) {
        return -EINVAL;
    } else if (*path != '/') {
        return -EINVAL;
    }

    /* Fetch filesystem capabilities if we can */
    if (vnode->fs != NULL) {
        fs = vnode->fs;
        fs_caps = fs->caps;
    }

    /*
     * If the filesystem requires full-path lookups, we can try
     * throwing the full path at the filesystem to see if
     * it'll give us a vnode.
     */
    if (__TEST(fs_caps, FSCAP_FULLPATH)) {
        s = vfs_vget(g_root_vnode, path, &vnode);
        goto done;
    }

    for (size_t i = 1;; ++i) {
        name = vfs_get_fname_at(path, i);
        if (name == NULL) break;

        s = vfs_vget(vnode, name, &vnode);
        dynfree(name);

        if (s != 0) break;
    }

done:
    if (vp != NULL && s == 0) {
        *vp = vnode;
    }

    return s;
}

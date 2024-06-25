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

#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <vm/dynalloc.h>
#include <string.h>

/*
 * Convert a path to a vnode.
 *
 * @ndp: Nameidata containing the path and resulting
 *       vnode.
 *
 * TODO: Add support for lookups with individual
 *       path components
 */
int
namei(struct nameidata *ndp)
{
    struct vnode *vp;
    struct vop_lookup_args lookup_args;
    const char *path = ndp->path;
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
     * Some filesystems (like initramfs) may only understand
     * full paths, so try passing it through.
     */
    lookup_args.name = path;
    lookup_args.dirvp = g_root_vnode;
    lookup_args.vpp = &vp;
    status = vfs_vop_lookup(lookup_args.dirvp, &lookup_args);

    if (status != 0) {
        return status;
    }

    ndp->vp = vp;
    return 0;
}

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

#ifndef _FS_TMPFS_H_
#define _FS_TMPFS_H_

#include <sys/types.h>
#include <sys/limits.h>
#include <sys/vnode.h>
#include <sys/queue.h>
#include <sys/spinlock.h>
#include <vm/vm_obj.h>

extern const struct vops g_tmpfs_vops;

/* Tmpfs node types */
#define TMPFS_NONE  (VNON)        /* No type */
#define TMPFS_REG   (VREG)        /* Regular file [f] */
#define TMPFS_DIR   (VDIR)        /* Directory    [d] */

struct tmpfs_node;

/*
 * A tmpfs node represents an object within the
 * tmpfs namespace such as a file, directory, etc.
 *
 * @rpath: /tmp/ relative path (for lookups)
 * @type: The tmpfs node type [one-to-one to vtype]
 * @len: Length of buffer
 * @real_size: Actual size of file
 * @data: The backing file data
 * @dirvp: Vnode of the parent node
 * @vp: Vnode of the current node
 * @lock: Lock protecting this node
 */
struct tmpfs_node {
    char rpath[PATH_MAX];
    uint8_t type;
    size_t len;
    size_t real_size;
    void *data;
    struct vnode *dirvp;
    struct vnode *vp;
    struct spinlock lock;
    TAILQ_HEAD(, tmpfs_node) dirents;
    TAILQ_ENTRY(tmpfs_node) link;
};

#endif  /* !_FS_TMPFS_H_ */

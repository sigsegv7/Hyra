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

#ifndef _SYS_VFS_H_
#define _SYS_VFS_H_

#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/types.h>
#include <sys/sio.h>

/* Max path length */
#define PATH_MAX 1024

#if defined(_KERNEL)

extern struct vnode *g_root_vnode;

void vfs_init(void);
struct fs_info *vfs_byname(const char *name);

int vfs_vget(struct vnode *parent, const char *name, struct vnode **vp);
int vfs_path_to_node(const char *path, struct vnode **vp);

char *vfs_get_fname_at(const char *path, size_t idx);
int vfs_rootname(const char *path, char **new_path);

bool vfs_is_valid_path(const char *path);
ssize_t vfs_hash_path(const char *path);

ssize_t vfs_read(struct vnode *vp, struct sio_txn *sio);
ssize_t vfs_write(struct vnode *vp, struct sio_txn *sio);

int vfs_getattr(struct vnode *vp, struct vattr *vattr);
int vfs_open(struct vnode *vp);

int vfs_close(struct vnode *vp);
uint64_t sys_mount(struct syscall_args *args);

#endif /* defined(_KERNEL) */
#endif  /* !_SYS_VFS_H_ */

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
#include <sys/errno.h>
#include <sys/mount.h>
#include <vm/dynalloc.h>
#include <string.h>

/* FNV-1a defines */
#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

/*
 * FNV-1a hash function
 * https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
 */
static size_t
vfs_hash(const char *data)
{
    size_t hash = FNV_OFFSET;
    const char *p = data;

    while (*p) {
        hash ^= (size_t)(uint8_t)(*p);
        hash *= FNV_PRIME;
        ++p;
    }

    return hash;
}

/*
 * Hashes a path
 *
 * @path: Path to hash.
 *
 * Returns -1 on failure, >= 0 return values
 * are valid.
 */
ssize_t
vfs_hash_path(const char *path)
{
    char *name = NULL;
    size_t i = 0, hash = 0;

    if (strcmp(path, "/") == 0 || !vfs_is_valid_path(path)) {
        return -1;
    }

    do {
        name = vfs_get_fname_at(path, i++);
        if (name != NULL) {
            hash += vfs_hash(name);
            dynfree(name);
        }
    } while (name != NULL);

    return hash;
}


/*
 * This function checks if a path is valid.
 *
 * @path: Path to check.
 *
 * Returns true if valid, otherwise false.
 */
bool
vfs_is_valid_path(const char *path)
{
    const char *ptr = path;
    char c;

    while (*ptr != '\0') {
        c = *(ptr++);
        switch (c) {
        case '/':
        case '-':
        case '_':
        case '.':
            /* Valid char, can continue */
            continue;
        default:
            /*
             * This could be an alphabetical or "numeric" char which
             * is valid. We want to handle this too. To make things
             * easier we'll OR the char by 0x20 to convert it to
             * lowercase if it is alphabetical.
             */
            c |= 0x20;
            if (c >= 'a' && c <= 'z')
                continue;
            else if (c >= '0' && c <= '9')
                continue;

            /* We got an invalid char */
            return false;
        }
    }

    return true;
}

/*
 * Allocates a vnode
 *
 * @vnode: Pointer to vnode pointer where newly allocated
 *         vnode will be stored.
 *
 * @mp: Mountpoint this vnode is associated with.
 * @type: Vnode type.
 *
 * This function will return 0 upon success and < 0 on failure.
 */
int
vfs_alloc_vnode(struct vnode **vnode, struct mount *mp, int type)
{
    struct vnode *new_vnode = dynalloc(sizeof(struct vnode));

    if (new_vnode == NULL) {
        return -ENOMEM;
    }

    memset(new_vnode, 0, sizeof(struct vnode));
    new_vnode->type = type;
    new_vnode->mp = mp;

    *vnode = new_vnode;
    return 0;
}

/*
 * Returns the rootname of a path
 * For example, "/foo/bar/" will be "foo" and
 * "/foo/bar/baz" will also be "foo"
 *
 * There will be no slashes in the returned string
 * unless "/" is passed.
 *
 * XXX: Returns memory allocated by dynalloc,
 *      remember to free the memory with dynfree()
 *
 * @path: Path to get rootname of
 * @new_path: New path will be placed here.
 */
int
vfs_rootname(const char *path, char **new_path)
{
    char *tmp = NULL;
    const char *ptr = path;
    size_t len = 0;

    if (!vfs_is_valid_path(path)) {
        *new_path = NULL;
        return -EINVAL;
    }

    if (*path == '/') {
        /* Skip first '/' */
        ++ptr;
        ++path;
    }

    for (; *ptr != '\0' && *ptr != '/'; ++ptr) {
        if (*ptr == '/') {
            break;
        }
        ++len;
    }

    tmp = dynalloc(sizeof(char) * len + 1);
    if (tmp == NULL) {
        *new_path = NULL;
        return -ENOMEM;
    }

    if (len == 0) {
        /* Handle input that is just "/" */
        tmp[0] = '/';
        tmp[1] = '\0';
    } else {
        memcpy(tmp, path, len + 2);
    }

    *new_path = tmp;
    return 0;
}


int
vfs_vget(struct vnode *parent, const char *name, struct vnode **vp)
{
    return parent->vops->vget(parent, name, vp);
}

ssize_t
vfs_read(struct vnode *vp, struct sio_txn *sio)
{
    return vp->vops->read(vp, sio);
}

ssize_t
vfs_write(struct vnode *vp, struct sio_txn *sio)
{
    return vp->vops->write(vp, sio);
}

int
vfs_getattr(struct vnode *vp, struct vattr *vattr)
{
    return vp->vops->getattr(vp, vattr);
}

int vfs_open(struct vnode *vp)
{
    if (vp->vops->open == NULL)
        return -EIO;

    return vp->vops->open(vp);
}

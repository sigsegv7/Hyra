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

#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/limits.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/filedesc.h>
#include <string.h>

static int
vfs_dostat(const char *path, struct stat *sbuf)
{
    char pathbuf[PATH_MAX];
    struct vattr *attr;
    struct stat st;
    struct vnode *vp;
    struct vop_getattr_args gattr;
    struct nameidata nd;
    int error;

    if (sbuf == NULL || path == NULL) {
        return -EINVAL;
    }

    if ((copyinstr(path, pathbuf, sizeof(path))) < 0) {
        return -EFAULT;
    }

    nd.path = path;
    nd.flags = 0;

    if ((error = namei(&nd)) != 0) {
        return error;
    }

    vp = nd.vp;
    gattr.vp = vp;
    error = vfs_vop_getattr(vp, &gattr);

    if (error != 0) {
        return error;
    }

    attr = gattr.res;
    memset(&st, VNOVAL, sizeof(st));

    /* Copy stat data to userspace statbuf */
    st.st_mode = attr->mode;
    st.st_size = attr->size;
    copyout(&st, sbuf, sizeof(*sbuf));
    return 0;
}

static int
vfs_doopen(const char *pathname, int flags)
{
    char pathbuf[PATH_MAX];

    if (copyinstr(pathname, pathbuf, PATH_MAX) < 0) {
        return -EFAULT;
    }

    return fd_open(pathbuf, flags);
}

/*
 * arg0: pathname
 * arg1: oflags
 *
 * TODO: Use oflags.
 */
scret_t
sys_open(struct syscall_args *scargs)
{
    return vfs_doopen((char *)scargs->arg0, scargs->arg1);
}

/*
 * arg0: fd
 */
scret_t
sys_close(struct syscall_args *args)
{
    return fd_close(args->arg0);
}

/*
 * arg0: fd
 * arg1: buf
 * arg2: count
 */
scret_t
sys_read(struct syscall_args *scargs)
{
    return fd_read(scargs->arg0, (void *)scargs->arg1,
        scargs->arg2);
}

/*
 * arg0: fd
 * arg1: buf
 * arg2: count
 */
scret_t
sys_write(struct syscall_args *scargs)
{
    return fd_write(scargs->arg0, (void *)scargs->arg1,
        scargs->arg2);
}

/*
 * arg0: path
 * arg1: buf
 */
scret_t
sys_stat(struct syscall_args *scargs)
{
    return vfs_dostat((const char *)scargs->arg0, (void *)scargs->arg1);
}

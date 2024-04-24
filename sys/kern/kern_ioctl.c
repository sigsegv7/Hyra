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

#include <sys/system.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/syscall.h>
#include <sys/errno.h>
#include <sys/sched.h>
#include <fs/devfs.h>

static int
do_ioctl(int fd, uint32_t cmd, uintptr_t arg)
{
    struct proc *td = this_td();
    struct filedesc *filedes;
    struct vnode *vp;
    struct device *dev;
    int status;

    filedes = fd_from_fdnum(td, fd);

    /* Fetch the vnode */
    if (filedes == NULL)
        return -EBADF;
    if ((vp = filedes->vnode) == NULL)
        return -EIO;

    if ((status = devfs_get_dev(vp, &dev)) != 0)
        return status;
    if (dev->ioctl == NULL)
        return -EIO;

    return dev->ioctl(dev, cmd, arg);
}

/*
 * Arg0: Fd.
 * Arg1: Cmd.
 * Arg2: Arg.
 */
uint64_t
sys_ioctl(struct syscall_args *args)
{
    return do_ioctl(args->arg0, args->arg1, args->arg2);
}

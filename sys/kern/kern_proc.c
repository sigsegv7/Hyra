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

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <sys/vnode.h>
#include <sys/tree.h>
#include <sys/syscall.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <string.h>
#include <crc32.h>

extern volatile size_t g_nthreads;

pid_t
getpid(void)
{
    struct proc *td;

    td = this_td();
    if (td == NULL) {
        return -1;
    }

    return td->pid;
}

pid_t
getppid(void)
{
    struct proc *td;

    td = this_td();
    if (td == NULL) {
        return -1;
    }
    if (td->parent == NULL) {
        return -1;
    }

    return td->parent->pid;
}

void
proc_coredump(struct proc *td, uintptr_t fault_addr)
{
    struct coredump core;
    struct sio_txn sio;
    struct vnode *vp;
    char pathname[128];
    int fd;

    snprintf(pathname, sizeof(pathname), "/tmp/core.%d", td->pid);
    fd = fd_open(pathname, O_RDWR | O_CREAT);

    /* ... Hopefully not */
    if (__unlikely(fd < 0)) {
        return;
    }

    core.pid = td->pid;
    core.fault_addr = fault_addr;
    memcpy(&core.tf, &td->tf, sizeof(td->tf));

    core.checksum = crc32(&core, sizeof(core) - sizeof(core.checksum));
    vp = fd_get(NULL, fd)->vp;

    sio.buf = &core;
    sio.len = sizeof(core);
    sio.offset = 0;

    /* Write the core file */
    vfs_vop_write(vp, &sio);
    fd_close(fd);
}

int
proc_init(struct proc *td, struct proc *parent)
{
    struct mmap_lgdr *mlgdr;

    mlgdr = dynalloc(sizeof(*mlgdr));
    if (mlgdr == NULL) {
        return -ENOMEM;
    }

    /* Add to parent leafq */
    TAILQ_INSERT_TAIL(&parent->leafq, td, leaf_link);
    atomic_inc_int(&parent->nleaves);
    atomic_inc_64(&g_nthreads);
    td->parent = parent;
    td->exit_status = -1;
    td->cred = parent->cred;

    /* Initialize the mmap ledger */
    mlgdr->nbytes = 0;
    RBT_INIT(lgdr_entries, &mlgdr->hd);
    td->mlgdr = mlgdr;
    td->flags |= PROC_WAITED;
    signals_init(td);
    return 0;
}

scret_t
sys_getpid(struct syscall_args *scargs)
{
    return getpid();
}

scret_t
sys_getppid(struct syscall_args *scargs)
{
    return getppid();
}

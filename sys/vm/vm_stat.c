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
#include <sys/errno.h>
#include <fs/ctlfs.h>
#include <vm/physmem.h>
#include <vm/vm.h>
#include <vm/stat.h>
#include <string.h>

#include <sys/syslog.h>

static struct ctlops vm_stat_ctl;

/*
 * ctlfs hook to read the virtual memory
 * statistics.
 */
static int
vm_stat_read(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    struct vm_stat stat;
    int error;

    if (sio->len > sizeof(stat)) {
        sio->len = sizeof(stat);
    }

    error = vm_stat_get(&stat);
    if (error < 0) {
        return error;
    }

    memcpy(sio->buf, &stat, sio->len);
    return sio->len;
}

int
vm_stat_get(struct vm_stat *vmstat)
{
    if (vmstat == NULL) {
        return -EINVAL;
    }

    vmstat->mem_avail = vm_mem_free();
    vmstat->mem_used = vm_mem_used();
    vmstat->mem_total = vm_mem_total();
    return 0;
}

void
vm_stat_init(void)
{
    char devname[] = "vm";
    struct ctlfs_dev ctl;

    /* Register a stat control file */
    ctl.mode = 0444;
    ctlfs_create_node(devname, &ctl);
    ctl.devname = devname;
    ctl.ops = &vm_stat_ctl;
    ctlfs_create_entry("stat", &ctl);
}

static struct ctlops vm_stat_ctl = {
    .read = vm_stat_read,
    .write = NULL
};

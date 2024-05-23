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

#include <sys/panic.h>
#include <machine/cpu.h>
#include <fs/procfs.h>
#include <vm/vm.h>
#include <string.h>

static bool populated = false;

static int
procfs_ver_read(struct proc_entry *p, struct sio_txn *sio)
{
    char buf[1024];
    size_t len;

    len = snprintf(buf, sizeof(buf), "Hyra/%s v%s: %s (%s)",
                   HYRA_ARCH, HYRA_VERSION,
                   HYRA_BUILDDATE, HYRA_BUILDBRANCH);

    /* Truncate if needed */
    if (len > sio->len)
        len = sio->len;

    memcpy(sio->buf, buf, len);
    return len;
}

static int
procfs_memstat_read(struct proc_entry *p, struct sio_txn *sio)
{
    struct vm_memstat stat;
    struct physmem_stat *pstat;
    char buf[1024];
    size_t len;

    stat = vm_memstat();
    pstat = &stat.pmem_stat;
    len = snprintf(buf, sizeof(buf),
                   "TotalMem:      %d KiB\n"
                   "ReservedMem:   %d KiB\n"
                   "AvailableMem:  %d KiB\n"
                   "AllocatedMem:  %d KiB\n"
                   "VMemObjCount:  %d",
                   pstat->total_kib,
                   pstat->reserved_kib,
                   pstat->avl_kib,
                   pstat->alloc_kib,
                   stat.vmobj_cnt);

    /* Truncate if needed */
    if (len > sio->len)
        len = sio->len;

    memcpy(sio->buf, buf, len);
    return len;
}

/*
 * Populate procfs with basic misc entries
 */
void
procfs_populate(void)
{
    struct proc_entry *version;
    struct proc_entry *memstat;

    if (populated)
        return;

    populated = true;

    /* Kernel version */
    version = procfs_alloc_entry();
    version->read = procfs_ver_read;
    procfs_add_entry("version", version);

    /* Memstat */
    memstat = procfs_alloc_entry();
    memstat->read = procfs_memstat_read;
    procfs_add_entry("memstat", memstat);
}

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

/*
 * System Accounting
 */

#include <sys/sched.h>
#include <sys/schedvar.h>
#include <sys/proc.h>
#include <fs/ctlfs.h>
#include <machine/cpu.h>
#include <string.h>

/* Called within kern_sched.c */
void sched_accnt_init(void);

static struct ctlops sched_stat_ctl;
volatile size_t g_nthreads;

static int
ctl_stat_read(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    struct sched_stat stat;

    if (sio->len > sizeof(stat)) {
        sio->len = sizeof(stat);
    }

    sched_stat(&stat);
    memcpy(sio->buf, &stat, sio->len);
    return sio->len;
}

/*
 * Get scheduler accounting information
 *
 * @statp: Info gets copied here
 */
void
sched_stat(struct sched_stat *statp)
{
    statp->nproc = atomic_load_64(&g_nthreads);
    statp->ncpu = cpu_count();
    statp->quantum_usec = DEFAULT_TIMESLICE_USEC;
}

void
sched_accnt_init(void)
{
    char devname[] = "sched";
    struct ctlfs_dev ctl;

    /*
     * Register some accounting information in
     * '/ctl/sched/stat'
     */
    ctl.mode = 0444;
    ctlfs_create_node(devname, &ctl);
    ctl.devname = devname;
    ctl.ops = &sched_stat_ctl;
    ctlfs_create_entry("stat", &ctl);
}

static struct ctlops sched_stat_ctl = {
    .read = ctl_stat_read,
    .write = NULL
};

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

#include <sys/intr.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <vm/dynalloc.h>
#include <fs/procfs.h>
#include <string.h>
#include <assert.h>

#define PROC_BUF_SIZE 4096

static TAILQ_HEAD(, intr_info) intrlist;
static struct mutex intrlist_lock = {0};
static struct proc_entry *proc;

static int
proc_read(struct proc_entry *entry, struct sio_txn *sio)
{
    struct intr_info *info;
    char buf[PROC_BUF_SIZE];
    char *p = &buf[0];
    size_t idx = 0, len;
    int res;

    mutex_acquire(&intrlist_lock);
    TAILQ_FOREACH(info, &intrlist, link) {
        __assert((sizeof(buf) - idx) > 0);
        res = snprintf(p, sizeof(buf) - idx,
                "CPU%d\t\t%d\t\t%s\t\t%s\n",
                info->affinity,
                info->count,
                info->source,
                info->device
        );

        if (res > 0) {
            idx += res;
            p += res;
        }
    }

    len = strlen(buf);
    if (sio->len > PROC_BUF_SIZE)
        sio->len = PROC_BUF_SIZE;
    if (len > sio->len)
        len = sio->len;

    memcpy(sio->buf, buf, len);
    mutex_release(&intrlist_lock);
    return sio->len;
}

/*
 * Register an interrupt stat
 *
 * @source: Source of interrupt (e.g IOAPIC)
 * @dev: Device (e.g i8042)
 */
struct intr_info *
intr_info_alloc(const char *source, const char *dev)
{
    struct intr_info *intr;

    intr = dynalloc(sizeof(*intr));
    if (intr == NULL)
        return NULL;

    memset(intr, 0, sizeof(*intr));
    intr->source = source;
    intr->device = dev;
    return intr;
}

void
intr_register(struct intr_info *info)
{
    if (info == NULL)
        return;

    TAILQ_INSERT_TAIL(&intrlist, info, link);
}

void
intr_init_proc(void)
{
    /* Init the interrupt list */
    TAILQ_INIT(&intrlist);

    /* Setup /proc/interrupts */
    proc = procfs_alloc_entry();
    proc->read = proc_read;
    procfs_add_entry("interrupts", proc);
}

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
#include <sys/param.h>
#include <sys/spinlock.h>
#include <sys/syslog.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <vm/physmem.h>
#include <vm/vm_pager.h>
#include <vm/vm_page.h>
#include <vm/vm.h>

#define pr_trace(fmt, ...) kprintf("vm_anon: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define ANON_TIMEOUT_USEC 200000

/*
 * Get pages from physical memory.
 *
 * @obp: Object representing the backing store (in memory).
 * @pgs: Page descriptors to be filled.
 * @off: Offset to read from in backing store.
 * @len: Length to read in bytes.
 */
static int
anon_get(struct vm_object *obp, struct vm_page **pgs, off_t off, size_t len)
{
    struct vm_page *pgtmp, *pgres;
    int retval = 0;
    size_t j, npgs;

    len = ALIGN_DOWN(len, DEFAULT_PAGESIZE);
    if (obp == NULL || pgs == NULL) {
        return -EINVAL;
    }

    /* Zero bytes is invalid */
    if (len == 0) {
        len = 4096;
    }

    spinlock_acquire(&obp->lock);
    npgs = len >> 12;

    for (int i = 0; i < npgs; ++i) {
        j = i / DEFAULT_PAGESIZE;
        pgtmp = vm_pagelookup(obp, i);
        pgres = pgs[j];

        /* Do we need to create our own entry? */
        if (pgtmp == NULL) {
            pgtmp = vm_pagealloc(obp, PALLOC_ZERO);
        }

        if (pgtmp == NULL) {
            pr_trace("anon_get: failed to add page %d, marking invalid\n", j);
            pgres->flags &= ~PG_VALID;
            continue;
        }

        /*
         * We are *just* populating `pgs' and therefore nobody
         * should even attempt to acquire this lock... Shit
         * happens though, so just make sure we can grab it
         * without assuming success.
         */
        if (spinlock_usleep(&pgres->lock, ANON_TIMEOUT_USEC) < 0) {
            vm_pagefree(obp, pgtmp, 0);
            pr_error("anon_get: pgres spin timeout\n");
            return -ETIMEDOUT;
        }

        /* Hold pgres before configuring it */
        spinlock_acquire(&pgres->lock);
        *pgres = *pgtmp;
        pgres->flags |= (PG_VALID | PG_CLEAN);
        spinlock_release(&pgres->lock);

    }

    spinlock_release(&obp->lock);
    return retval;
}

const struct vm_pagerops vm_anonops = {
    .get = anon_get
};

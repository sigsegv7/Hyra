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
#include <dev/dcdr/cache.h>
#include <vm/dynalloc.h>
#include <string.h>

/*
 * Remove a DCD from a DCDR.
 */
static void
dcdr_remove(struct dcdr *dcdr, struct dcd *dcd)
{
    /* Handle the head being invalidated */
    if (dcd == dcdr->head) {
        dcdr->head = dcd->next;
        return;
    }

    /* Handle the tail being invalidated */
    if (dcd == dcdr->tail) {
        dcdr->tail = dcd->prev;
        return;
    }

    if (dcd->prev != NULL) {
        dcd->prev->next = dcd->next;
    }

    if (dcd->next != NULL) {
        dcd->next->prev = dcd->prev;
    }

    if (dcdr->size > 0) {
        --dcdr->size;
    }
}

/*
 * Evict a DCD with the lowest hit count
 * from a DCDR.
 */
static void
dcdr_evict_least(struct dcdr *dcdr)
{
    struct dcd *tmp = dcdr->head;
    struct dcd *cnp = NULL;    /* Candidate */

    /* Find DCD eviction candidate */
    while (tmp != NULL) {
        if (cnp == NULL) {
            cnp = tmp;
            continue;
        }

        if (tmp->hit_count < cnp->hit_count) {
            cnp = tmp;
        }

        tmp = tmp->next;
    }

    /* Invalidate DCD found if any */
    if (cnp != NULL) {
        dcdr_invldcd(dcdr, cnp->lba);
    }
}

/*
 * Allocates a DCDR structure using a
 * specific block size.
 *
 * Returns NULL on failure.
 */
struct dcdr *
dcdr_alloc(size_t bsize, size_t cap)
{
    struct dcdr *tmp;

    tmp = dynalloc(sizeof(*tmp));
    if (tmp == NULL) {
        return NULL;
    }

    tmp->bsize = bsize;
    tmp->head = NULL;
    tmp->tail = NULL;
    tmp->cap = cap;
    tmp->size = 0;
    return tmp;
}

/*
 * Cache a logical block and return a DCD that
 * describes it. This will allocate a block and
 * copy `block` to the allocated memory.
 */
struct dcd *
dcdr_cachein(struct dcdr *dcdr, void *block, off_t lba)
{
    struct dcd *dcd, *tmp;
    struct dcdr_lookup check;
    int status;

    /*
     * If there is already a block within this
     * DCDR, then we simply need to copy the
     * new data into the old DCD.
     */
    status = dcdr_lookup(dcdr, lba, &check);
    if (status == 0) {
        dcd = check.dcd_res;
        memcpy(dcd->block, block, dcdr->bsize);
        return dcd;
    }

    dcd = dynalloc(sizeof(*dcd));
    if (dcd == NULL) {
        return NULL;
    }

    memset(dcd, 0, sizeof(*dcd));
    dcd->block = dynalloc(dcdr->bsize);
    if (dcd->block == NULL) {
        dynfree(dcd);
        return NULL;
    }

    dcd->lba = lba;
    dcd->next = NULL;
    dcd->hit_count = 0;
    memcpy(block, dcd->block, dcdr->bsize);

    /*
     * If we've hit the capacity of the DCDR then
     * we will need to evict some DCD to make room
     * for this new one.
     */
    if (dcdr->size >= dcdr->cap) {
        dcdr_evict_least(dcdr);
    }

    /* Insert DCD into DCDR */
    if (dcdr->head == NULL) {
        dcdr->head = dcd;
        dcdr->tail = dcd;
    } else {
        tmp = dcdr->tail;
        dcd->prev = tmp;
        tmp->next = dcd;
        dcdr->tail = dcd;
    }

    ++dcdr->size;
    return dcd;
}

/*
 * Cache two consecutive logical blocks and
 * return the DCD describing the first.
 *
 * XXX: 'block' *must* span two LBAs
 */
struct dcd *
dcdr_lbc_cachein(struct dcdr *dcdr, void *block, off_t lba)
{
    struct dcd *tmp;

    tmp = dcdr_cachein(dcdr, block, lba);
    dcdr_cachein(dcdr, (char *)block + dcdr->bsize, lba);
    tmp->lbc = 1;
    return tmp;
}

/*
 * Search for a logical block within the cache.
 *
 * Returns 0 upon a cache hit with "res" being set
 * and returns a less than 0 value upon a cache miss.
 */
int
dcdr_lookup(struct dcdr *dcdr, off_t lba, struct dcdr_lookup *res)
{
    struct dcdr_lookup ret;
    struct dcd *tmp = dcdr->head;

    while (tmp != NULL) {
        if (tmp->lba == lba) {
            ++tmp->hit_count;
            ret.dcd_res = tmp;
            ret.lba = lba;
            ret.buf = tmp->block;
            *res = ret;
            return 0;
        }

        if (tmp->lbc && tmp->lba == (lba - 1)) {
            ++tmp->hit_count;
            ret.dcd_res = tmp;
            ret.lba = lba;
            ret.buf = (char *)tmp->block + dcdr->bsize;
            *res = ret;
            return 0;
        }

        tmp = tmp->next;
    }

    return -1;
}

/*
 * Invalidate a DCD within a DCDR.
 */
int
dcdr_invldcd(struct dcdr *dcdr, off_t lba)
{
    struct dcdr_lookup tmp;
    struct dcd *dcd;
    int error;

    if ((error = dcdr_lookup(dcdr, lba, &tmp)) < 0) {
        return error;
    }

    dcd = tmp.dcd_res;
    dcdr_remove(dcdr, dcd);

    dynfree(dcd->block);
    dynfree(dcd);
    return 0;
}

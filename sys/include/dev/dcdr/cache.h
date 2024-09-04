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

#ifndef _DCDR_CACHE_H_
#define _DCDR_CACHE_H_

#include <sys/types.h>

/*
 * A drive cache descriptor (DCD) describes a logical
 * block within a storage medium and is chained with
 * other DCDs. Logical block coalescing is a method
 * to optimize caching by combining adjacent logical block
 * pairs that are infrequently invalidated into a single
 * DCD.
 *
 * If the lbc bit is set, this block is coalesced with
 * the next.
 */
struct dcd {
    off_t lba;          /* Starting LBA */
    void *data;         /* Driver specific data */
    void *block;        /* Cached data from described block */
    uint8_t lbc : 1;    /* Set if coalesced */
    uint32_t hit_count; /* Number of hits */
    struct dcd *next;   /* Next ptr for DCD chaining */
    struct dcd *prev;   /* Prev ptr for DCD chaining */
};

/*
 * This structure describes a drive cache descriptor
 * ring and contains basic information like the size
 * of each block.
 */
struct dcdr {
    size_t bsize;       /* Block size */
    size_t cap;         /* Capacity (in entries) */
    size_t size;        /* Size (in entries) */
    struct dcd *head;   /* Ring head */
    struct dcd *tail;   /* Ring tail */
};

/*
 * Result from DCDR lookup.
 */
struct dcdr_lookup {
    struct dcd *dcd_res;
    void *buf;
    off_t lba;
};

struct dcdr *dcdr_alloc(size_t bsize, size_t cap);
struct dcd *dcdr_cachein(struct dcdr *dcdr, void *block, off_t lba);

struct dcd *dcdr_lbc_cachein(struct dcdr *dcdr, void *block, off_t lba);
int dcdr_lookup(struct dcdr *dcdr, off_t lba, struct dcdr_lookup *res);
int dcdr_invldcd(struct dcdr *dcdr, off_t lba);

#endif  /* !_DCDR_CACHE_H_ */

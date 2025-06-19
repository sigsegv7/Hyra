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
#include <machine/vas.h>
#include <vm/pmap.h>

struct vas
pmap_read_vas(void)
{
    /* TODO: STUB */
    struct vas vas = {0};
    return vas;
}

void
pmap_switch_vas(struct vas vas)
{
    /* TODO: STUB */
    return;
}

int
pmap_map(struct vas vas, vaddr_t va, paddr_t pa, vm_prot_t prot)
{
    /* TODO: STUB */
    return 0;
}

int
pmap_unmap(struct vas vas, vaddr_t va)
{
    /* TODO: STUB */
    return 0;
}

void
pmap_destroy_vas(struct vas vas)
{
    /* TODO: STUB */
    return;
}

bool
pmap_is_clean(struct vas vas, vaddr_t va)
{
    /* TODO: STUB */
    return false;
}

void
pmap_mark_clean(struct vas vas, vaddr_t va)
{
    /* TODO: STUB */
    return;
}

int
pmap_set_cache(struct vas vas, vaddr_t va, int type)
{
    /* TODO: STUB */
    return 0;
}

int
pmap_init(void)
{
    uint64_t mair;

    mair = MT_ATTR(MT_NORMAL, MEM_NORMAL)       |
           MT_ATTR(MT_NORMAL_UC, MEM_NORMAL_UC) |
           MT_ATTR(MT_DEVICE, MEM_DEV_NGNRNE);
    mair_el1_write(mair);
    return 0;
}

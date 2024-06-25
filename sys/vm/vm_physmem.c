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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/limine.h>
#include <sys/syslog.h>
#include <sys/spinlock.h>
#include <vm/physmem.h>
#include <vm/vm.h>
#include <string.h>

size_t highest_frame_idx = 0;
size_t bitmap_size = 0;
size_t bitmap_free_start = 0;

uint8_t *bitmap;
static struct limine_memmap_response *resp = NULL;
static struct spinlock lock = {0};

static struct limine_memmap_request mmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

/*
 * Populate physical memory bitmap.
 */
static void
physmem_populate_bitmap(void)
{
    struct limine_memmap_entry *ent;

    for (size_t i = 0; i < resp->entry_count; ++i) {
        ent = resp->entries[i];

        if (ent->type != LIMINE_MEMMAP_USABLE) {
            /* This memory is not usable */
            continue;
        }

        if (bitmap_free_start == 0) {
            bitmap_free_start = ent->base / DEFAULT_PAGESIZE;
        }

        for (size_t j = 0; j < ent->length; j += DEFAULT_PAGESIZE) {
            clrbit(bitmap, (ent->base + j) / DEFAULT_PAGESIZE);
        }
    }
}

/*
 * Allocate physical memory for the bitmap
 * we'll use to keep track of free memory.
 */
static void
physmem_alloc_bitmap(void)
{
    struct limine_memmap_entry *ent;
    uintptr_t highest_addr = 0;

    for (size_t i = 0; i < resp->entry_count; ++i) {
        ent = resp->entries[i];

        if (ent->type != LIMINE_MEMMAP_USABLE) {
            /* This memory is not usable */
            continue;
        }

        if (ent->length >= bitmap_size) {
            bitmap = PHYS_TO_VIRT(ent->base);
            memset(bitmap, 0xFF, bitmap_size);
            ent->length -= bitmap_size;
            ent->base += bitmap_size;
            return;
        }

        highest_addr = MAX(highest_addr, ent->base + ent->length);
    }
}


/*
 * Init the physical memory bitmap.
 */
static void
physmem_init_bitmap(void)
{
    uintptr_t highest_addr = 0;
    struct limine_memmap_entry *ent;

    for (size_t i = 0; i < resp->entry_count; ++i) {
        ent = resp->entries[i];

        if (ent->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        highest_addr = MAX(highest_addr, ent->base + ent->length);
    }

    highest_frame_idx = highest_addr / DEFAULT_PAGESIZE;
    bitmap_size = ALIGN_UP(highest_frame_idx / 8, DEFAULT_PAGESIZE);

    physmem_alloc_bitmap();
    physmem_populate_bitmap();
}

/*
 * Allocate page frames.
 *
 * @count: Number of frames to allocate.
 */
uintptr_t
vm_alloc_frame(size_t count)
{
    size_t frames = 0;
    uintptr_t ret = 0;

    spinlock_acquire(&lock);
    for (size_t i = 0; i < highest_frame_idx; ++i) {
        if (!testbit(bitmap, i)) {
            /* We have a free page */
            if (++frames != count) {
                continue;
            }

            for (size_t j = i; j < i + count; ++j) {
                setbit(bitmap, j);
            }

            ret = i * DEFAULT_PAGESIZE;
            break;
        }
    }

    spinlock_release(&lock);
    return ret;
}

void
vm_free_frame(uintptr_t base, size_t count)
{
    size_t stop_at = base + (count * DEFAULT_PAGESIZE);

    spinlock_acquire(&lock);
    for (uintptr_t p = base; p < stop_at; p += DEFAULT_PAGESIZE) {
        clrbit(bitmap, p / DEFAULT_PAGESIZE);
    }
    spinlock_release(&lock);
}

void
vm_physmem_init(void)
{
    resp = mmap_req.response;
    physmem_init_bitmap();
}

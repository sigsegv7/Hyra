/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

#include <mm/phys_mgr.h>
#include <mm/vm.h>
#include <sys/limine.h>
#include <sys/module.h>
#include <sys/panic.h>
#include <sys/printk.h>
#include <sync/spinlock.h>
#include <string.h>
#include <bitmap.h>
#include <math.h>

MODULE("phys_mgr");

volatile struct limine_memmap_request mmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static struct limine_memmap_response *mmap_resp = NULL;
static size_t bitmap_size = 0;
static size_t bitmap_free_start = 0;
static bitmap_t bitmap = NULL;

static void
phys_mgr_alloc_bitmap(void)
{
    for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
        struct limine_memmap_entry *entry = mmap_resp->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        if (entry->length >= bitmap_size) {
            bitmap = (bitmap_t)(entry->base + VM_HIGHER_HALF);
            memset(bitmap, 0xFF, bitmap_size);
            entry->length -= bitmap_size;
            entry->base += bitmap_size;
            return;
        }
    }
}

static void
phys_mgr_populate_bitmap(void)
{
    for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
        struct limine_memmap_entry *entry = mmap_resp->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        if (bitmap_free_start == 0) {
            bitmap_free_start = entry->base / 0x1000;
        }

        for (size_t j = 0; j < entry->length; j += 0x1000) {
            bitmap_unset_bit(bitmap, (entry->base + j) / 0x1000);
        }
    }
}

static void
phys_mgr_init_bitmap(void)
{
    uintptr_t highest_addr = 0;
    size_t highest_page_index = 0;

    /* Find the highest address */
    for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
        struct limine_memmap_entry *entry = mmap_resp->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        highest_addr = MAX(highest_addr, entry->base + entry->length);
    }

    highest_page_index = highest_addr / 0x1000;
    bitmap_size = ALIGN_UP(highest_page_index / 8, 0x1000);

    kinfo("Highest physical address: 0x%x\n", highest_addr);
    kinfo("Bitmap is of size %d bytes\n", bitmap_size);

    phys_mgr_alloc_bitmap();
    phys_mgr_populate_bitmap();
}

size_t
get_phys_mem_mib(void)
{
    static bool has_cached_val = false;
    static size_t cached_val = 0; 
    size_t size_bytes = 0;

    if (has_cached_val) {
        return cached_val;
    }

    for (size_t i = 0; i < mmap_resp->entry_count; ++i) {
        struct limine_memmap_entry *entry = mmap_resp->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        size_bytes += entry->length;
    }

    cached_val = size_bytes/MIB;
    return cached_val;
}

/*
 * Allocates a contigous
 * chunk of memory.
 */

uintptr_t
phys_mgr_alloc(size_t frame_count)
{
    if (frame_count == 0) {
        return 0;
    }

    size_t bitmap_end = bitmap_free_start+(bitmap_size*8);

    size_t found_bit = 0;
    size_t found_frames = 0;

    for (size_t i = bitmap_free_start; i < bitmap_end; ++i) {
        if (bitmap_test_bit(bitmap, i) != 0) {
            /* Non-free memory, reset values */
            found_bit = 0;
            found_frames = 0;
            continue;
        }

        if (found_bit == 0) {
            found_bit = i;
        }

        
        if ((found_frames++) == frame_count) {
            /* We found the required amount of frames */
            break;
        }

        for (size_t j = found_bit; j < found_bit + frame_count; ++j) {
            bitmap_set_bit(bitmap, i);
        }

        return found_bit*0x1000;
    }

    return 0;
}

void
phys_mgr_free(uintptr_t phys, size_t frame_count)
{
    size_t start_bit = phys/0x1000;

    for (size_t i = start_bit; i < start_bit+frame_count; ++i) {
        bitmap_unset_bit(bitmap, i);
    }
}

void
phys_mgr_init(void)
{
    mmap_resp = mmap_req.response;
    size_t mem_mib = get_phys_mem_mib();

    if (mem_mib > 1024) {
        kinfo("System has %d GiB of memory\n", mem_mib/1024);
    } else {
        kinfo("System has %d MiB of memory\n", mem_mib);
    }

    if (mem_mib < 512) {
        panic("System is deadlocked on memory (mem=%d MiB)\n", mem_mib);
    }

    phys_mgr_init_bitmap();
}

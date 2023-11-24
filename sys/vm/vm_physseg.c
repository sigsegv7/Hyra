/*
 * Copyright (c) 2023 Ian Marco Moffett and the Osmora Team.
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

#include <sys/limine.h>
#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <vm/physseg.h>
#include <vm/vm.h>
#include <bitmap.h>
#include <string.h>

__MODULE_NAME("vm_physseg");
__KERNEL_META("$Hyra$: vm_physseg.c, Ian Marco Moffett, "
              "The Hyra physical memory manager");

#if defined(VM_PHYSSEG_DEBUG)
#define DPRINTF(...) KDEBUG(__VA_ARGS__)
#else
#define DPRINTF(...) __nothing
#endif      /* defined(VM_PHYSSEG_DEBUG) */

static struct limine_memmap_request mmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static struct limine_memmap_response *resp = NULL;

__used static const char *segment_name[] = {
    [LIMINE_MEMMAP_USABLE]                  = "usable",
    [LIMINE_MEMMAP_RESERVED]                = "reserved",
    [LIMINE_MEMMAP_ACPI_RECLAIMABLE]        = "ACPI reclaimable",
    [LIMINE_MEMMAP_ACPI_NVS]                = "ACPI NVS",
    [LIMINE_MEMMAP_BAD_MEMORY]              = "bad",
    [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE]  = "bootloader reclaimable",
    [LIMINE_MEMMAP_KERNEL_AND_MODULES]      = "kernel and modules",
    [LIMINE_MEMMAP_FRAMEBUFFER]             = "framebuffer"
};

static const int MAX_SEGMENTS = __ARRAY_COUNT(segment_name);


static bitmap_t bitmap = NULL;
static size_t last_used_idx = 0;
static size_t bitmap_size = 0;
static size_t highest_frame_idx;
static size_t bitmap_free_start;    /* Beginning bit of free region */

static void
vm_physseg_bitmap_alloc(void)
{
    struct limine_memmap_entry *entry;
    uintptr_t highest_addr = 0;

    for (size_t i = 0; i < resp->entry_count; ++i) {
        entry = resp->entries[i];

        /* Drop any entries with an invalid type */
        if (entry->type >= MAX_SEGMENTS) {
            continue;
        }

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            /* This memory is not usable */
            continue;
        }

        if (entry->length >= bitmap_size) {
            bitmap = PHYS_TO_VIRT(entry->base);
            memset(bitmap, 0xFF, bitmap_size);
            entry->length -= bitmap_size;
            entry->base += bitmap_size;
            return;
        }

        highest_addr = __MAX(highest_addr, entry->base + entry->length);
    }
}

static void
vm_physseg_bitmap_populate(void)
{
    struct limine_memmap_entry *entry;
#if defined(VM_PHYSSEG_DEBUG)
    size_t start, end;
#endif      /* defined(VM_PHYSSEG_DEBUG) */

    for (size_t i = 0; i < resp->entry_count; ++i) {
        entry = resp->entries[i];

        /* Drop any entries with an invalid type */
        if (entry->type >= MAX_SEGMENTS) {
            continue;
        }

#if defined(VM_PHYSSEG_DEBUG)
        /* Dump the memory map if we are debugging */
        start = entry->base;
        end = entry->base + entry->length;
        DPRINTF("0x%x - 0x%x, size: 0x%x, type: %s\n",
                start, end, entry->length,
                segment_name[entry->type]);
#endif  /* defined(VM_PHYSSEG_DEBUG) */

        /* Don't set non-usable entries as free */
        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        /* Populate */
        if (bitmap_free_start == 0) {
            bitmap_free_start = entry->base/0x1000;
        }
        for (size_t j = 0; j < entry->length; j += 0x1000) {
            bitmap_unset_bit(bitmap, (entry->base + j) / 0x1000);
        }
    }
}

static void
vm_physseg_bitmap_init(void)
{
    uintptr_t highest_addr;
    struct limine_memmap_entry *entry;

    highest_addr = 0;
    highest_frame_idx = 0;

    /* Find the highest entry */
    for (size_t i = 0; i < resp->entry_count; ++i) {
        entry = resp->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            /* Memeory not usable */
            continue;
        }

        highest_addr = __MAX(highest_addr, entry->base + entry->length);
    }

    highest_frame_idx = highest_addr / 0x1000;
    bitmap_size = __ALIGN_UP(highest_frame_idx / 8, 0x1000);

    DPRINTF("Bitmap size: %d bytes\n", bitmap_size);
    DPRINTF("Allocating and populating bitmap now...\n");

    vm_physseg_bitmap_alloc();
    vm_physseg_bitmap_populate();
}

static uintptr_t
vm_alloc_pageframe_internal(size_t count)
{
    bool can_alloc = false;

    bool is_free;               /* True if we found a free frame */
    size_t free_count = 0;      /* How many we found that are free? */
    uintptr_t frame_idx = 0;    /* The base index of first free frame */

    for (size_t i = last_used_idx; i < bitmap_size*8; ++i) {
        is_free = !bitmap_test_bit(bitmap, i);

        if (!is_free) {
            free_count = 0;
            continue;
        }

        if (free_count == count) {
            can_alloc = true;
            break;
        }

        /* Assume free here */
        if (frame_idx == 0)
            frame_idx = i;

        ++free_count;
    }

    if (!can_alloc) {
        /* No free memory! */
        last_used_idx = 0;
        return 0;
    }

    /* Mark the memory as allocated */
    for (size_t i = frame_idx; i < frame_idx+count; ++i) {
        bitmap_set_bit(bitmap, i);
    }

    /* Return the physical address */
    last_used_idx = frame_idx;
    return frame_idx*vm_get_page_size();
}

/*
 * Allocates physical pageframes.
 *
 * @count: Number of pageframes to allocate.
 */
uintptr_t
vm_alloc_pageframe(size_t count)
{
    uintptr_t phys;

    phys = vm_alloc_pageframe_internal(count);

    if (phys == 0)
        return 0;

    vm_zero_page(PHYS_TO_VIRT(phys), count);
    return phys;
}

/*
 * Frees physical pageframes.
 *
 * @base: Base to start freeing at.
 * @count: Number of pageframes to free.
 */
void
vm_free_pageframe(uintptr_t base, size_t count)
{
    const size_t PAGE_SIZE = vm_get_page_size();

    for (uintptr_t p = base; p < base + (count*PAGE_SIZE); p += PAGE_SIZE) {
        bitmap_unset_bit(bitmap, p/0x1000);
    }
}

void
vm_physseg_init(void)
{
    resp = mmap_req.response;

    vm_physseg_bitmap_init();
}

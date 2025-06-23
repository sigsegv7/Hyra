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
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdbool.h>

#define HEAP_SIZE   0x1001A8
#define HEAP_MAGIC  0x05306A    /* "OSMORA" :~) */
#define HEAP_PROT PROT_READ | PROT_WRITE
#define BYTE_PTR(PTR) ((char *)(PTR))
#define HEAP_NEXT(BLOCKP, SIZE) \
    PTR_OFFSET((BLOCKP), sizeof(struct mem_block) + (SIZE))

struct __aligned(4) mem_block {
    uint32_t magic;
    uint32_t size;
    uint8_t allocated : 1;
    struct mem_block *next;
};

static struct mem_block *mem_head;
static struct mem_block *mem_tail;

/*
 * The size of the heap including data on the heap
 * as well as the sizes of their respective block
 * headers.
 */
static ssize_t heap_len = 0;
static off_t heap_pos = 0;

/*
 * During the initial state of malloc() when the C library
 * first starts up. We can assume that there is zero fragmentation
 * in our heap pool. This allows us to initially allocate memory
 * by bumping a pointer which is O(1). During this state, even after
 * any calls to free(), we can assume that there is more memory ahead
 * of us that is free (due to the initial zero-fragmentation). However,
 * once we've reached the end of the pool, if any memory has been previous
 * freed (indicated by heap_len > 0), we can wrap the tail and start allocating
 * in a best-fit fasion as we assume that heap is now fragmented.
 */
static bool wrap = false;

void __malloc_mem_init(void);

/*
 * Terminate program abnormally due to any heap
 * errors.
 *
 * TODO: Raise SIGABRT instead of using _Exit()
 */
__dead static void
__heap_abort(const char *str)
{
    printf(str);
    _Exit(1);
    __builtin_unreachable();
}

/*
 * Find a free block
 *
 * TODO: This is currently a first-fit style
 *       routine. This should be best-fit instead
 *       as it doesn't waste memory.
 */
static struct mem_block *
malloc_find_free(size_t size)
{
    struct mem_block *cur = mem_head;

    while (cur != NULL) {
        if (cur->size >= size) {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
}

void *
malloc(size_t size)
{
    struct mem_block *next_block;
    struct mem_block *tail;
    size_t inc_len = 0;

    size = ALIGN_UP(size, 4);
    inc_len = sizeof(*next_block) + size;

    if (heap_len < 0) {
        heap_len = 0;
    }

    if (heap_len < 0) {
        heap_len = 0;
        return NULL;
    }

    /* Any memory left to allocate? */
    if ((heap_len + inc_len) >= HEAP_SIZE) {
        return NULL;
    }

    if (heap_pos >= HEAP_SIZE - inc_len) {
        wrap = true;
        mem_tail = mem_head;
    }

    tail = wrap ? malloc_find_free(size) : mem_tail;
    if (tail == NULL) {
        return NULL;
    }

    next_block = mem_tail;
    mem_tail = HEAP_NEXT(mem_tail, size);
    mem_tail->next = NULL;

    next_block->next = mem_tail;
    next_block->size = size;
    next_block->allocated = 1;
    next_block->magic = HEAP_MAGIC;

    heap_len += inc_len;
    heap_pos += inc_len;
    return PTR_OFFSET(next_block, sizeof(*next_block));
}

void
free(void *ptr)
{
    struct mem_block *blk;

    blk = PTR_NOFFSET(ptr, sizeof(*blk));
    if (blk->magic != HEAP_MAGIC) {
        __heap_abort("free: bad free block detected\n");
    }
    if (!blk->allocated) {
        __heap_abort("free: double free detected\n");
    }

    blk->allocated = 0;
    heap_len -= (blk->size + sizeof(*blk));
    if (heap_len < 0) {
        heap_len = 0;
    }
}

void
__malloc_mem_init(void)
{
    mem_head = mmap(NULL, HEAP_SIZE, HEAP_PROT, MAP_ANON, 0, 0);
    if (mem_head == NULL) {
        __heap_abort("__malloc_mem_init: mem_head is NULL, out of memory\n");
    }

    mem_head->size = 0;
    mem_head->next = NULL;
    mem_head->allocated = 0;
    mem_tail = mem_head;
}

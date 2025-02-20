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

#ifndef _MACHINE_BUS_H_
#define _MACHINE_BUS_H_

#include <sys/types.h>
#include <sys/spinlock.h>
#include <sys/param.h>

struct bus_resource;

/*
 * Hyra assumes that the bootloader uses PDE[256] for some
 * higher half mappings. To avoid conflicts with those mappings,
 * this offset is used to start device memory at PDE[257]. This
 * will give us more than enough space.
 */
#define MMIO_OFFSET (VM_HIGHER_HALF + 0x8000000000)

/* Resource signature size max */
#define RSIG_MAX 16

/*
 * Basic bus resource semantics
 *
 * BUS_PIO: If set, this resource uses port I/O
 * BUS_MMIO: If set, this resource uses memory-mapped I/O
 * BUS_WRITABLE: If unset, this resource is read-only
 * BUS_DMA: If set, this resource is DMA-capable
 */
#define BUS_PIO       BIT(0)
#define BUS_MMIO      BIT(1)
#define BUS_WRITABLE  BIT(2)
#define BUS_DMA       BIT(3)

/*
 * Common bus types.
 *
 * bus_addr_t: Physical MMIO address
 * bus_sem_t: Resource semantics
 */
typedef uint64_t bus_addr_t;
typedef uint64_t bus_sem_t;

struct bus_op {
    /* Enable/disable DMA */
    int(*enable_dma)(struct bus_resource *brp, void *arg);
    int(*disable_dma)(struct bus_resource *brp, void *arg);

    /* Set/unset flags */
    int(*set_sem)(struct bus_resource *brp, bus_sem_t sem);
    int(*clr_sem)(struct bus_resource *brp, bus_sem_t sem);

    /* DMA buffer related operations */
    int(*dma_alloc)(struct bus_resource *brp, void *res);
    int(*dma_free)(struct bus_resource *brp, void *p);

    /* DMA transfer related operations */
    ssize_t(*dma_in)(struct bus_resource *brp, void *p);
    ssize_t(*dma_out)(struct bus_resource *brp, void *p);
};

struct bus_resource {
    char signature[RSIG_MAX];   /* e.g., "PCI\0", "ISA\0", "LPC\0", etc */
    off_t align;                /* Alignment required (0: none) */
    bus_addr_t dma_max;         /* Maximum address possible for DMA */
    bus_addr_t dma_min;         /* Minimum address possible for DMA */
    bus_addr_t base;            /* Resource base [physical] address */
    bus_sem_t sem;              /* Resource semantics */
    struct bus_op io;           /* I/O operations */
    struct spinlock lock;       /* Protects this structure */
};

int bus_map(bus_addr_t addr, size_t size, int flags, void **vap);
struct bus_resource *bus_establish(const char *name);

#endif  /* !_MACHINE_BUS_H_ */

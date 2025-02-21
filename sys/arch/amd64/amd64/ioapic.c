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
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/mmio.h>
#include <machine/ioapicvar.h>
#include <machine/ioapic.h>
#include <dev/acpi/tables.h>
#include <dev/acpi/acpi.h>

#define pr_trace(fmt, ...) kprintf("ioapic: " fmt, ##__VA_ARGS__)
#define IOAPIC_BASE_OFF(off) ((void *)((uintptr_t)ioapic_base + off))

static void *ioapic_base = NULL;
static struct acpi_madt *madt = NULL;

/*
 * Converts IRQ numbers to its corresponding
 * Global System Interrupt (GSI) number.
 *
 * @irq: IRQ number.
 */
static uint32_t
irq_to_gsi(uint8_t irq)
{
    struct apic_header *hdr = NULL;
    struct interrupt_override *override = NULL;
    uint8_t *cur = NULL;
    uint8_t *end = NULL;

    if (madt == NULL)
        madt = acpi_query("APIC");
    if (madt == NULL)
        panic("failed to fetch MADT\n");

    cur = (uint8_t *)(madt + 1);
    end = (uint8_t *)madt + madt->hdr.length;

    while (cur < end) {
        hdr = (void *)cur;

        if (hdr->type == APIC_TYPE_INTERRUPT_OVERRIDE) {
            override = (struct interrupt_override *)cur;
            if (override->source == irq) {
                return override->interrupt;
            }
        }

        cur += hdr->length;
    }

    return irq;
}

/*
 * Reads a 32 bit value from the IOAPIC
 * register space.
 *
 * @reg: Register to read from.
 */
static uint32_t
ioapic_readl(uint16_t reg)
{
    mmio_write32(IOAPIC_BASE_OFF(IOREGSEL), reg);
    return mmio_read32(IOAPIC_BASE_OFF(IOWIN));
}

/*
 * Writes a 32 bit value to the IOAPIC
 * register space.
 *
 * @reg: Register to write to.
 * @val: Value to write.
 */
static void
ioapic_writel(uint16_t reg, uint32_t val)
{
    mmio_write32(IOAPIC_BASE_OFF(IOREGSEL), reg);
    mmio_write32(IOAPIC_BASE_OFF(IOWIN), val);
}

/*
 * Reads an I/O APIC redirection entry.
 *
 * @entry: Entry variable to read into.
 * @index: Index to read.
 */
static void
ioapic_read_redentry(union ioapic_redentry *entry, uint8_t index)
{
    uint32_t lo, hi;

    lo = ioapic_readl(IOREDTBL + index * 2);
    hi = ioapic_readl(IOREDTBL + index * 2 + 1);
    entry->value = ((uint64_t)hi << 32) | lo;
}

/*
 * Writes an I/O APIC redirection entry.
 *
 * @entry: Entry variable to write.
 * @index: Index to write to.
 */
static void
ioapic_write_redentry(const union ioapic_redentry *entry, uint8_t index)
{
    ioapic_writel(IOREDTBL + index * 2, (uint32_t)entry->value);
    ioapic_writel(IOREDTBL + index * 2 + 1, (uint32_t)(entry->value >> 32));
}

/*
 * Mask I/O APIC pin with "raw" pin number
 * (Global System Interrupt)
 *
 * @gsi: Global System Interrupt number
 */
void
ioapic_gsi_mask(uint8_t gsi)
{
    union ioapic_redentry redentry;

    ioapic_read_redentry(&redentry, gsi);
    redentry.interrupt_mask = 1;
    ioapic_write_redentry(&redentry, gsi);
}

/*
 * Unmask I/O APIC pin with "raw" pin number
 * (Global System Interrupt)
 *
 * @gsi: Global System Interrupt number
 */
void
ioapic_gsi_unmask(uint8_t gsi)
{
    union ioapic_redentry redentry;

    ioapic_read_redentry(&redentry, gsi);
    redentry.interrupt_mask = 0;
    ioapic_write_redentry(&redentry, gsi);
}

/*
 * Masks I/O APIC pin via IRQ number
 *
 * @irq: Interrupt Request number
 */
void
ioapic_irq_mask(uint8_t irq)
{
    uint8_t gsi = irq_to_gsi(irq);
    ioapic_gsi_mask(gsi);
}

/*
 * Unmasks I/O APIC pin via IRQ number
 *
 * @irq: Interrupt Request number
 */
void
ioapic_irq_unmask(uint8_t irq)
{
    uint8_t gsi = irq_to_gsi(irq);
    ioapic_gsi_unmask(gsi);
}

/*
 * Assign an interrupt vector to a redirection
 * entry.
 *
 * @irq: IRQ number to assign vector to.
 * @vector: Vector assign.
 */
void
ioapic_set_vec(uint8_t irq, uint8_t vector)
{
    union ioapic_redentry redentry;
    uint8_t gsi = irq_to_gsi(irq);

    ioapic_read_redentry(&redentry, gsi);
    redentry.vector = vector;
    ioapic_write_redentry(&redentry, gsi);
}

void
ioapic_init(struct ioapic *p)
{
    size_t tmp;
    uint8_t redir_ent_cnt, ver;

    ioapic_base = (void *)((uintptr_t)p->ioapic_addr);
    tmp = ioapic_readl(IOAPICVER);
    ver = tmp & 0xFF;
    redir_ent_cnt = ((tmp >> 16) & 0xFF) + 1;

    for (uint8_t i = 0; i < redir_ent_cnt; ++i) {
        ioapic_gsi_mask(i);
    }

    pr_trace("ioapic0 at mainbus0: ver %d, addr %p\n", ver, ioapic_base);
    pr_trace("%d GSIs masked\n", redir_ent_cnt);
}

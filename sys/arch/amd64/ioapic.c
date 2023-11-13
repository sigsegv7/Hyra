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

#include <machine/ioapic.h>
#include <machine/ioapicvar.h>
#include <firmware/acpi/acpi.h>
#include <sys/panic.h>
#include <sys/mmio.h>
#include <sys/cdefs.h>
#include <sys/syslog.h>

__MODULE_NAME("ioapic");
__KERNEL_META("$Hyra$: ioapic.c, Ian Marco Moffett, "
              "I/O APIC driver");

#define IOAPIC_BASE_OFF(off) ((void *)((uintptr_t)ioapic_base + off))

static void *ioapic_base = NULL;

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
    uint8_t gsi;

    gsi = irq_to_gsi(irq);
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
    uint8_t gsi;

    gsi = irq_to_gsi(irq);
    ioapic_gsi_unmask(gsi);
}

void
ioapic_set_base(void *mmio_base)
{
    if (ioapic_base == NULL)
        ioapic_base = mmio_base;
}

void
ioapic_init(void)
{
    size_t tmp;
    uint8_t redir_entry_cnt;

    /* Sanity check */
    if (ioapic_base == NULL)
        panic("ioapic base not set!\n");

    tmp = ioapic_readl(IOAPICVER);
    redir_entry_cnt = __SHIFTOUT(tmp, 0xFF << 16) + 1;

    KINFO("Masking %d GSIs...\n", redir_entry_cnt);

    for (uint8_t i = 0; i < redir_entry_cnt; ++i) {
        ioapic_gsi_mask(i);
    }
}

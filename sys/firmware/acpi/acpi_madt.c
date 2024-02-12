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

#include <firmware/acpi/acpi.h>
#include <firmware/acpi/tables.h>
#include <machine/ioapic.h>
#include <machine/lapic.h>
#include <machine/cpu.h>
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/syslog.h>

#define APIC_TYPE_LOCAL_APIC            0
#define APIC_TYPE_IO_APIC               1
#define APIC_TYPE_INTERRUPT_OVERRIDE    2

__MODULE_NAME("acpi");
__KERNEL_META("$Hyra$: acpi_madt.c, Ian Marco Moffett, "
              "ACPI MADT parsing");

static struct acpi_madt *madt = NULL;

void *
acpi_get_lapic_base(void)
{
    if (madt == NULL)
        return NULL;
    return (void *)(uint64_t)madt->lapic_addr;
}

static void
do_parse(struct cpu_info *ci)
{
    uint8_t *cur = NULL;
    uint8_t *end = NULL;

    void *ioapic_mmio_base = NULL;

    struct apic_header *hdr = NULL;
    struct ioapic *ioapic = NULL;

    cur = (uint8_t *)(madt + 1);
    end = (uint8_t *)madt + madt->hdr.length;

    /* Parse the rest of the MADT */
    while (cur < end) {
        hdr = (void *)cur;

        switch (hdr->type) {
        case APIC_TYPE_IO_APIC:
            /*
             * TODO: Figure out how to use multiple
             *       I/O APICs.
             */
            if (ioapic != NULL) {
                break;
            }

            ioapic = (struct ioapic *)cur;

            KINFO("Detected I/O APIC (id=%d, gsi_base=%d)\n",
                  ioapic->ioapic_id, ioapic->gsi_base);

            ioapic_mmio_base = (void *)(uintptr_t)ioapic->ioapic_addr;
            ioapic_set_base(ioapic_mmio_base);
            break;
        }

        cur += hdr->length;
    }
}

/*
 * Converts IRQ numbers to its corresponding
 * Global System Interrupt (GSI) number.
 *
 * @irq: IRQ number.
 */
uint32_t
irq_to_gsi(uint8_t irq)
{
    struct apic_header *hdr = NULL;
    struct interrupt_override *override = NULL;
    uint8_t *cur = NULL;
    uint8_t *end = NULL;

    cur = (uint8_t *)(madt + 1);
    end = (uint8_t *)madt + madt->hdr.length;

    while (cur < end) {
        hdr = (void *)cur;

        switch (hdr->type) {
        case APIC_TYPE_INTERRUPT_OVERRIDE:
            override = (struct interrupt_override *)cur;
            if (override->source == irq) {
                return override->interrupt;
            }
        }

        cur += hdr->length;
    }

    return irq;
}

void
acpi_parse_madt(struct cpu_info *ci)
{
    /* Prevent this function from running twice */
    if (madt != NULL) {
        return;
    }

    madt = acpi_query("APIC");
    if (madt == NULL) {
        panic("Failed to query for ACPI MADT\n");
    }

    do_parse(ci);
}

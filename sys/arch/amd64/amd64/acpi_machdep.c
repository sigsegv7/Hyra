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

#include <sys/panic.h>
#include <sys/syslog.h>
#include <dev/acpi/acpi.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/tables.h>
#include <machine/ioapic.h>
#include <machine/lapic.h>
#include <vm/vm.h>

#define pr_trace(fmt, ...) kprintf("acpi: " fmt, ##__VA_ARGS__)

int
acpi_init_madt(void)
{
    struct acpi_madt *madt = acpi_query("APIC");
    struct apic_header *apichdr;
    struct ioapic *ioapic = NULL;
    uint8_t *cur, *end;

    if (madt == NULL) {
        panic("could not find MADT!\n");
    }

    cur = (uint8_t *)(madt + 1);
    end = (uint8_t *)madt + madt->hdr.length;
    g_lapic_base = PHYS_TO_VIRT(madt->lapic_addr);

    while (cur < end) {
        apichdr = (void *)cur;
        if (apichdr->type == APIC_TYPE_IO_APIC) {
            /*
             * TODO: Figure out how to use multiple I/O APICs
             */
            if (ioapic != NULL) {
                pr_trace("skipping I/O APIC with ID %d\n", ioapic->ioapic_id);
                break;
            }

            ioapic = (struct ioapic *)cur;
            ioapic_init(ioapic);
        }

        cur += apichdr->length;
    }

    return 0;
}

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

#include <sys/types.h>
#include <dev/acpi/acpi.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/tables.h>
#include <vm/vm.h>
#include <string.h>

/*
 * Compute the ACPI checksum of a header.
 *
 * @hdr: HDR to check.
 *
 * Must return zero to be a valid checksum!
 */
uint8_t
acpi_checksum(struct acpi_header *hdr)
{
    uint8_t sum = 0;

    for (int i = 0; i < hdr->length; ++i) {
        sum += ((char *)hdr)[i];
    }

    return sum;
}

/*
 * Looks up an ACPI table with a specific
 * signature e.g "APIC" for MADT (if present).
 *
 * @query: The specific query to make e.g "APIC"
 */
void *
acpi_query(const char *query)
{
    struct acpi_header *hdr;
    struct acpi_root_sdt *root_sdt;
    size_t root_sdt_len, signature_len;

    root_sdt = acpi_get_root_sdt();
    root_sdt_len = acpi_get_root_sdt_len();
    signature_len = sizeof(hdr->signature);

    for (size_t i = 0; i < root_sdt_len; ++i) {
        hdr = (struct acpi_header *)PHYS_TO_VIRT(root_sdt->tables[i]);

        if (memcmp(hdr->signature, query, signature_len) == 0) {
            return (void *)hdr;
        }
    }

    return NULL;
}

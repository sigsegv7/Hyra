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
#include <sys/limine.h>
#include <sys/syslog.h>
#include <sys/cdefs.h>
#include <sys/panic.h>
#include <sys/syslog.h>
#include <vm/vm.h>

__MODULE_NAME("acpi");
__KERNEL_META("$Hyra$: acpi_init.c, Ian Marco Moffett, "
              "ACPI init logic");

#define pr_trace(fmt, ...) kprintf("acpi: " fmt, ##__VA_ARGS__)

static volatile struct limine_rsdp_request rsdp_req = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static size_t root_sdt_entries = 0;
static bool using_xsdt = false;
static struct acpi_root_sdt *root_sdt = NULL;

/*
 * Writes out OEMID of ACPI header.
 *
 * @type: Type of structure (e.g RSDP)
 * @hdr: Header of structure.
 */
static void
acpi_print_oemid(const char *type, char oemid[OEMID_SIZE])
{
    if (type != NULL) {
        pr_trace("%s OEMID: ", type);
    }

    for (size_t i = 0; i < OEMID_SIZE; ++i) {
        kprintf(OMIT_TIMESTAMP "%c", oemid[i]);
    }
    kprintf(OMIT_TIMESTAMP "\n");
}

struct acpi_root_sdt *
acpi_get_root_sdt(void)
{
    return root_sdt;
}

size_t
acpi_get_root_sdt_len(void)
{
    return root_sdt_entries;
}

void
acpi_init(void)
{
    struct acpi_rsdp *rsdp;

    /* Can't do anything if we have no response! */
    if (rsdp_req.response == NULL) {
        panic("RSDP request has no response affiliated...\n");
    }

    /* Fetch the RSDP */
    rsdp = rsdp_req.response->address;
    acpi_print_oemid("RSDP", rsdp->oemid);

    /* Fetch the RSDT/XSDT  */
    if (rsdp->revision >= 2) {
        using_xsdt = true;
        root_sdt = PHYS_TO_VIRT(rsdp->xsdt_addr);
        pr_trace("Using XSDT as root SDT\n");
    } else {
        root_sdt = PHYS_TO_VIRT(rsdp->rsdt_addr);
        pr_trace("Using RSDT as root SDT\n");
    }
    if (!acpi_is_checksum_valid(&root_sdt->hdr)) {
        panic("Root SDT has an invalid checksum!\n");
    }
    root_sdt_entries = (root_sdt->hdr.length - sizeof(root_sdt->hdr)) / 4;
}

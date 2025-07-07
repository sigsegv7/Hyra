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
#include <sys/limine.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/driver.h>
#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <dev/dmi/dmi.h>
#include <dev/acpi/tables.h>
#include <string.h>

#define DMI_BIOS_INFO       0
#define DMI_SYSTEM_INFO     1
#define DMI_PROCESSOR_INFO  4
#define DMI_END_OF_TABLE    127

/* String offsets */
#define BIOSINFO_VENDOR     0x01
#define SYSINFO_PRODUCT     0x02
#define SYSINFO_FAMILY      0x03
#define PROCINFO_MANUFACT   0x02
#define PROCINFO_PARTNO     0x06

static struct limine_smbios_request smbios_req = {
    .id = LIMINE_SMBIOS_REQUEST,
    .revision = 0
};

/* DMI/SMBIOS structure header */
struct __packed dmi_shdr {
    uint8_t type;
    uint8_t length;
    uint16_t handle;
} *hdrs[DMI_END_OF_TABLE + 1];

/*
 * Grab a structure header from a type
 *
 * @type: A DMI structure type to find
 *
 * Returns NULL if not found.
 */
static inline struct dmi_shdr *
dmi_shdr(uint8_t type)
{
    struct dmi_shdr *hdr;

    hdr = hdrs[type];
    if (hdr == NULL) {
        return NULL;
    }

    return hdr;
}

/*
 * Grab a string from the DMI/SMBIOS formatted
 * section.
 *
 * @hdr: DMI header to lookup string index
 * @index: 1-based string index
 *
 * See section 6.1.3 of the DTMF SMBIOS Reference
 * Specification
 */
static const char *
dmi_str_index(struct dmi_shdr *hdr, uint8_t index)
{
    const char *strdata = PTR_OFFSET(hdr, hdr->length);

    for (uint8_t i = 1; *strdata != '\0'; ++i) {
        if (i == index) {
            return strdata;
        }

        strdata += strlen(strdata) + 1;
    }

    return NULL;
}

/*
 * Get the DMI/SMBIOS structure size from a
 * header.
 */
static size_t
dmi_struct_size(struct dmi_shdr *hdr)
{
    const char *strdata;
    size_t i = 1;

    strdata = PTR_OFFSET(hdr, hdr->length);
    while (strdata[i - 1] != '\0' || strdata[i] != '\0') {
        ++i;
    }

    return hdr->length + i + 1;
}

/*
 * Get the vendor string from the DMI/SMBIOS BIOS
 * info structure
 *
 * Returns NULL if not found.
 */
const char *
dmi_vendor(void)
{
    struct dmi_shdr *hdr;

    if ((hdr = dmi_shdr(DMI_BIOS_INFO)) == NULL) {
        return NULL;
    }

    return dmi_str_index(hdr, BIOSINFO_VENDOR);
}

/*
 * Return the product string from the DMI/SMBIOS System
 * Info structure
 *
 * Returns NULL if not found.
 */
const char *
dmi_product(void)
{
    struct dmi_shdr *hdr;

    if ((hdr = dmi_shdr(DMI_SYSTEM_INFO)) == NULL) {
        return NULL;
    }

    return dmi_str_index(hdr, SYSINFO_PRODUCT);
}

/*
 * Return the product version from the DMI/SMBIOS
 * System Info structure
 *
 * Returns NULL if not found
 */
const char *
dmi_prodver(void)
{
    struct dmi_shdr *hdr;

    if ((hdr = dmi_shdr(DMI_SYSTEM_INFO)) == NULL) {
        return NULL;
    }

    return dmi_str_index(hdr, SYSINFO_FAMILY);
}

/*
 * Return the CPU manufacturer string from the
 * DMI/SMBIOS Processor Info structure
 *
 * Returns NULL if not found
 */
const char *
dmi_cpu_manufact(void)
{
    struct dmi_shdr *hdr;

    if ((hdr = dmi_shdr(DMI_PROCESSOR_INFO)) == NULL) {
        return NULL;
    }

    return dmi_str_index(hdr, PROCINFO_MANUFACT);
}

static int
dmi_init(void)
{
    struct dmi_entry32 *entry32 = NULL;
    struct limine_smbios_response *resp = smbios_req.response;
    struct dmi_entry64 *entry64 = NULL;
    struct dmi_shdr *hdr = NULL;
    size_t scount = 0, smax_len = 0;
    size_t nbytes = 0, cur_nbytes = 0;

    if (resp == NULL) {
        return -ENODEV;
    }
    if (resp->entry_32 == 0 && resp->entry_64 == 0) {
        return -ENODEV;
    }

    if (resp->entry_64 != 0) {
        entry64 = (void *)resp->entry_64;
        hdr = PHYS_TO_VIRT(entry64->addr);
        smax_len = entry64->max_size;
    } else if (resp->entry_32 != 0) {
        entry32 = (void *)(uint64_t)resp->entry_32;
        hdr = PHYS_TO_VIRT((uint64_t)entry32->addr);
        scount = entry32->nstruct;
    } else {
        return -ENODEV;
    }

    memset(hdrs, 0, sizeof(hdrs));
    for (size_t i = 0; i < scount; ++i) {
        if (hdr->type == DMI_END_OF_TABLE) {
            break;
        }

        if (hdr->type < NELEM(hdrs)) {
            hdrs[hdr->type] = hdr;
        }
        cur_nbytes = dmi_struct_size(hdr);
        if (smax_len > 0 && (nbytes + cur_nbytes) >= smax_len) {
            break;
        }

        nbytes += cur_nbytes;
        hdr = PTR_OFFSET(hdr, cur_nbytes);
    }

    return 0;
}

DRIVER_EXPORT(dmi_init, "dmi");

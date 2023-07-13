/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

/* $Id$ */

#ifndef _ACPI_TABLES_H_
#define _ACPI_TABLES_H_

#include <sys/types.h>
#include <sys/cdefs.h>

#define OEMID_SIZE 6

struct __packed acpi_header {
    uint32_t signature;         /* ASCII signature string */
    uint32_t length;            /* Length of table in bytes */
    uint8_t revision;           /* Revision of the structure */
    uint8_t checksum;           /* Checksum of the header */
    char oemid[OEMID_SIZE];     /* OEM-supplied string that IDs the OEM */
    char oem_table_id[8];       /* OEM-supplied string (used by OEM) */
    uint32_t oem_revision;      /* OEM-supplied revision number */
    uint32_t creator_id;        /* Vendor ID of creator utility */
    uint32_t creator_revision;  /* Revision of creator utility */
};

struct __packed acpi_rsdp {
    uint64_t signature;         /* RSD PTR */
    uint8_t checksum;           /* Structure checksum */
    char oemid[OEMID_SIZE];     /* OEM-supplied string that IDs the OEM */
    uint8_t revision;           /* Revision of the structure */ 
    uint32_t rsdt_addr;         /* RSDT physical address */

    /* Reserved if revision < 2 */
    uint32_t length;            /* Length of table in bytes */
    uint64_t xsdt_addr;         /* XSDT physical address */
    uint8_t ext_checksum;       /* Extended checksum */
    uint8_t reserved[3];
};

/* 
 * XSDT or RSDT depending
 * on what revision the header
 * says.
 */
struct __packed acpi_root_sdt {
    struct acpi_header hdr;
    void *entries;              /* 8*n */
};

#endif      /* !_ACPI_TABLES_H_ */

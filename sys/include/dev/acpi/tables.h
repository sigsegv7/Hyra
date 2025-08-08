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

#ifndef _ACPI_TABLES_H_
#define _ACPI_TABLES_H_

#include <sys/types.h>
#include <sys/cdefs.h>

/* MADT APIC header types */
#define APIC_TYPE_LOCAL_APIC            0
#define APIC_TYPE_IO_APIC               1
#define APIC_TYPE_INTERRUPT_OVERRIDE    2

#define OEMID_SIZE 6

struct __packed acpi_header {
    char signature[4];          /* ASCII signature string */
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
    uint32_t tables[];
};

struct __packed acpi_madt {
    struct acpi_header hdr;
    uint32_t lapic_addr;
    uint32_t flags;
};

struct __packed apic_header {
    uint8_t type;
    uint8_t length;
};

struct __packed local_apic {
    struct apic_header hdr;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
};

struct __packed ioapic {
    struct apic_header hdr;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_addr;
    uint32_t gsi_base;
};

struct __packed interrupt_override {
    struct apic_header hdr;
    uint8_t bus;
    uint8_t source;         /* IRQ */
    uint32_t interrupt;     /* GSI */
    uint16_t flags;
};

struct __packed acpi_gas {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
};

/*
 * ACPI Address Space ID definitions for GAS
 *
 * See section 5.2.3.2 of the ACPI software programming
 * manual.
 *
 * XXX: 0x0B->0x7E is reserved as well as 0x80->0xBF
 *      and 0xC0->0xFF is OEM defined. Values other than
 *      the ones specified below are either garbage or
 *      OEM specific values.
 */
#define ACPI_GAS_SYSMEM   0x00      /* System memory space */
#define ACPI_GAS_SYSIO    0x01      /* System I/O space */
#define ACPI_GAS_PCICONF  0x02      /* PCI configuration space */
#define ACPI_GAS_EC       0x03      /* Embedded controller */
#define ACPI_GAS_SMBUS    0x04      /* System management bus */
#define ACPI_GAS_CMOS     0x05      /* System CMOS */
#define ACPI_GAS_PCIBAR   0x06      /* PCI BAR target */
#define ACPI_GAS_IPMI     0x07      /* IPMI (sensor monitoring) */
#define ACPI_GAS_GPIO     0x08      /* General Purpose I/O */
#define ACPI_GAS_GSBUS    0x09      /* GenericSerialBus */
#define ACPI_GAS_PLATCOM  0x0A      /* Platform Communications Channel */

/*
 * ACPI address size definitions for GAS
 *
 * See section 5.2.3.2 of the ACPI software programming
 * manual.
 *
 * This is really retarded Intel and Microsoft, thank you.
 */
#define ACPI_GAS_UNDEF  0   /* Undefined (legacy reasons) */
#define ACPI_GAS_BYTE   1   /* Byte access */
#define ACPI_GAS_WORD   2   /* Word access */
#define ACPI_GAS_DWORD  3   /* Dword access */
#define ACPI_GAS_QWORD  4   /* Qword access */

struct __packed acpi_hpet {
    struct acpi_header hdr;
    uint8_t hardware_rev_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved : 1;
    uint8_t legacy_replacement : 1;
    uint16_t pci_vendor_id;
    struct acpi_gas gas;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
};

/*
 * PCIe / ACPI MCFG base address description
 * table.
 *
 * @base_pa: Enhanced configuration base [physical]
 * @seg_grpno: PCI segment group number
 * @bus_start: Host bridge bus start
 * @bus_end: Host bridge bus end
 */
struct __packed acpi_mcfg_base {
    uint64_t base_pa;
    uint16_t seg_grpno;
    uint8_t bus_start;
    uint8_t bus_end;
    uint32_t reserved;
};

/*
 * PCIe / ACPI MCFG structure
 *
 * @hdr: ACPI header
 * @reserved: Do not use
 * @base: ECAM MMIO address list
 */
struct __packed acpi_mcfg {
    struct acpi_header hdr;
    uint32_t reserved[2];
    struct acpi_mcfg_base base[1];
};

struct __packed dmi_entry32 {
    char signature[4];      /* _SM_ */
    uint8_t checksum;       /* Sum of table bytes */
    uint8_t length;         /* Length of entry table */
    uint8_t major;          /* DMI major */
    uint8_t minor;          /* DMI minor */
    uint16_t max_size;      /* Max structure size */
    uint8_t rev;            /* Entry revision */
    char fmt_area[5];       /* Formatted area */
    char isignature[5];     /* Intermediate signature */
    uint8_t ichecksum;      /* Intermediate checksum */
    uint16_t table_len;     /* Length of SMBIOS structure table */
    uint32_t addr;          /* 32-bit physical start of SMBIOS structure table */
    uint16_t nstruct;       /* Total number of structures */
    uint8_t bcd_rev;
};

struct __packed dmi_entry64 {
    char signature[5];      /* _SM_ */
    uint8_t checksum;       /* Sum of table bytes */
    uint8_t length;         /* Length of entry table */
    uint8_t major;          /* DMI major */
    uint8_t minor;          /* DMI minor */
    uint8_t docrev;
    uint8_t entry_rev;
    uint8_t reserved;
    uint16_t max_size;      /* Max structure size */
    uint16_t padding;
    uint64_t addr;          /* 64-bit physical address */
};

#endif  /* _ACPI_TABLES_H_ */

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

#ifndef _AHCIVAR_H_
#define _AHCIVAR_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/mutex.h>
#include <dev/ic/ahciregs.h>

struct ata_identity {
    uint16_t rsvd0      : 1;
    uint16_t unused0    : 1;
    uint16_t incomplete : 1;
    uint16_t unused1    : 3;
    uint16_t fixed_dev  : 1;
    uint16_t removable  : 1;
    uint16_t unused2    : 7;
    uint16_t device_type : 1;
    uint16_t ncylinders;
    uint16_t specific_config;
    uint16_t nheads;
    uint16_t unused3[2];
    uint16_t sectors_per_track;
    uint16_t vendor[3];
    char serial_number[20];
    uint16_t unused4[2];
    uint16_t unused5;
    char firmware_rev[8];
    char model_number[40];
    char pad[256];
};

/* Physical region descriptor table entry */
struct ahci_prdt_entry {
    uintptr_t dba;          /* Data base address */
    uint32_t rsvd0;         /* Reserved */
    uint32_t dbc : 22;      /* Count */
    uint16_t rsvd1 : 9;     /* Reserved */
    uint8_t i : 1;          /* Interrupt on completion */
};

/* Command header */
struct ahci_cmd_hdr {
    uint8_t cfl : 5;            /* Command FIS length */
    uint8_t a   : 1;            /* ATAPI */
    uint8_t w   : 1;            /* Write */
    uint8_t p   : 1;            /* Prefetchable */
    uint8_t r   : 1;            /* Reset */
    uint8_t c   : 1;            /* Clear busy upon R_OK */
    uint8_t rsvd0 : 1;          /* Reserved */
    uint8_t pmp   : 4;          /* Port multiplier port */
    uint16_t prdtl;             /* PRDT length (in entries) */
    volatile uint32_t prdbc;    /* PRDT bytes transferred count */
    uintptr_t ctba;             /* Command table descriptor base addr */
    uint32_t rsvd1[4];          /* Reserved */
};

/* Command table */
struct ahci_cmdtab {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsvd[48];
    struct ahci_prdt_entry prdt[1];
};

struct ahci_fis_h2d {
    uint8_t type;
    uint8_t pmp : 4;
    uint8_t rsvd0 : 3;
    uint8_t c : 1;
    uint8_t command;
    uint8_t featurel;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    uint8_t rsvd1[4];
};

struct ahci_hba {
    struct hba_memspace *abar;
    struct ahci_cmd_hdr *cmdlist;
    uint32_t ncmdslots;
    uint32_t nports;
    struct mutex lock;
};

/* Commands */
#define ATA_CMD_NOP      0x00
#define ATA_CMD_IDENTIFY 0xEC

/* FIS types */
#define FIS_TYPE_H2D 0x27
#define FIS_TYPE_D2H 0x34

#define AHCI_TIMEOUT 500
#define AHCI_FIS_SIZE 256
#define AHCI_CMDTAB_SIZE 256
#define AHCI_CMDENTRY_SIZE 32

#endif  /* !_AHCIVAR_H_ */

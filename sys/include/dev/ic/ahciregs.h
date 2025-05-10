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

#ifndef _IC_AHCIREGS_H_
#define _IC_AHCIREGS_H_

#include <sys/types.h>
#include <sys/param.h>

struct hba_port {
    volatile uint32_t clb;      /* Command list base low (1k-byte aligned) */
    volatile uint32_t clbu;     /* Command list base upper */
    volatile uint32_t fb;       /* FIS base (256-byte aligned) */
    volatile uint32_t fbu;      /* FIS base upper */
    volatile uint32_t is;       /* Interrupt status */
    volatile uint32_t ie;       /* Interrupt enable */
    volatile uint32_t cmd;      /* Command and status */
    volatile uint32_t rsvd0;    /* Reserved */
    volatile uint32_t tfd;      /* Task file data */
    volatile uint32_t sig;      /* Signature */
    volatile uint32_t ssts;     /* SATA status */
    volatile uint32_t sctl;     /* SATA control */
    volatile uint32_t serr;     /* SATA error */
    volatile uint32_t sact;     /* SATA active */
    volatile uint32_t ci;       /* Command issue */
    volatile uint32_t sntf;     /* SATA notification */
    volatile uint32_t fbs;      /* FIS-based switch control */
    volatile uint32_t rsvd1[11];
    volatile uint32_t vendor[4];
};

struct hba_memspace {
    volatile uint32_t cap;          /* Host Capabilities */
    volatile uint32_t ghc;          /* Global host control */
    volatile uint32_t is;           /* Interrupt status */
    volatile uint32_t pi;           /* Ports implemented */
    volatile uint32_t vs;           /* Version */
    volatile uint32_t ccc_ctl;      /* Command completion coalescing control */
    volatile uint32_t ccc_pts;      /* Command completion coalescing ports */
    volatile uint32_t em_loc;       /* Enclosure management location */
    volatile uint32_t em_ctl;       /* Enclosure management control */
    volatile uint32_t cap2;         /* Host capabilities extended */
    volatile uint32_t bohc;         /* BIOS/OS Handoff Control and Status */
    volatile uint8_t rsvd[0x74];    /* Reserved */
    volatile uint8_t vendor[0x60];  /* Vendor specific */
    struct hba_port ports[1];
};

/* Global host control bits */
#define AHCI_GHC_AE BIT(31)   /* AHCI enable */
#define AHCI_GHC_IE BIT(1)    /* Interrupt enable */
#define AHCI_GHC_HR BIT(0)    /* HBA reset */

/* AHCI port signatures */
#define AHCI_SIG_ATA    0x00000101
#define AHCI_SIG_SEMB   0xC33C0101
#define AHCI_SIG_PM     0x96690101

/*
 * Port SATA status
 * See section 3.3.10 of the AHCI spec.
 */
#define AHCI_PXSSTS_DET(SSTS) (SSTS & 0xF)
#define AHCI_PXSSTS_IPM(SSTS) ((SSTS >> 8) & 0xF)

/*
 * Port SATA control bits
 * See section 3.3.11 of the AHCI spec.
 */
#define AHCI_PXSCTL_DET(SCTL) (SCTL & 0xF)

/*
 * Port command and status bits
 * See section 3.3.7 of the AHCI spec.
 */
#define AHCI_PXCMD_ST   BIT(0)    /* Start */
#define AHCI_PXCMD_FRE  BIT(4)    /* FIS Receive Enable */
#define AHCI_PXCMD_FR   BIT(14)   /* FIS Receive Running  */
#define AHCI_PXCMD_CR   BIT(15)   /* Command List Running */

/*
 * Interrupt status bits
 * See section 3.3.5 of the AHCI spec.
 */
#define AHCI_PXIS_TFES BIT(31)

/*
 * Task file data bits
 * See section 3.3.8 of the AHCI spec.
 */
#define AHCI_PXTFD_ERR BIT(0)
#define AHCI_PXTFD_DRQ BIT(3)
#define AHCI_PXTFD_BSY BIT(7)

/*
 * Capability bits
 * See section 3.1.1 of the AHCI spec.
 */
#define AHCI_CAP_NP(CAP) (CAP & 0x1F)           /* Number of ports */
#define AHCI_CAP_NCS(CAP) ((CAP >> 8) & 0x1F)   /* Number of command slots */
#define AHCI_CAP_EMS(CAP) ((CAP >> 6) & 1)      /* Enclosure management support */
#define AHCI_CAP_SAL(CAP) ((CAP >> 25) & 1)     /* Supports activity LED */
#define AHCI_CAP_SSS(CAP) ((CAP >> 27) & 1)     /* Supports staggered spin up */

/*
 * Device detection (DET) and Interface power
 * management (IPM) values
 * See section 3.3.10 of the AHCI spec.
 */
#define AHCI_DET_NULL       0   /* No device detected */
#define AHCI_DET_PRESENT    1   /* Device present (no PHY comm) */
#define AHCI_DET_COMM       3   /* Device present and phy comm established */
#define AHCI_IPM_ACTIVE     1

/*
 * Device detection initialization values
 * See section 3.3.11 of the AHCI spec.
 */
#define AHCI_DET_COMRESET 1

#endif  /* !_IC_AHCIREGS_H_ */

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

#ifndef _USB_XHCIREGS_H_
#define _USB_XHCIREGS_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cdefs.h>

/*
 * Host Controller Capability Registers
 *
 * See xHCI spec, section 5.3, table 5-9
 */
struct xhci_caps {
    volatile uint8_t caplength;
    volatile uint8_t reserved;
    volatile uint16_t hciversion;
    volatile uint32_t hcsparams1;
    volatile uint32_t hcsparams2;
    volatile uint32_t hcsparams3;
    volatile uint32_t hccparams1;
    volatile uint32_t dboff;
    volatile uint32_t rtsoff;
    volatile uint32_t hccparams2;
};

/*
 * Host Controller Operational Registers
 *
 * See xHCI spec, section 5.4, table 5-18
 */
struct xhci_opregs {
    volatile uint32_t usbcmd;
    volatile uint32_t usbsts;
    volatile uint32_t pagesize;
    volatile uint32_t reserved;
    volatile uint32_t reserved1;
    volatile uint32_t dnctrl;
    volatile uint64_t cmd_ring;
    volatile uint32_t reserved2[4];
    volatile uint64_t dcbaa_ptr;
    volatile uint32_t config;
};

/* USBCMD bits */
#define USBCMD_RUN      BIT(0)    /* Run/stop */
#define USBCMD_HCRST    BIT(1)    /* xHC reset */
#define USBCMD_INTE     BIT(2)    /* Interrupt Enable */

/* USBSTS bits */
#define USBSTS_HCH      BIT(0)    /* HC halted */

/* CAPS.HCSPARAMS1 fields */
#define XHCI_MAXSLOTS(HCSPARAMS1) (HCSPARAMS1 & 0xFF)
#define XHCI_MAXPORTS(HCSPARAMS1) ((HCSPARAMS1 >> 24) & 0xFF)
#define XHCI_ECP(HCCPARAMS1) ((HCCPARAMS1 >> 16) & 0xFFFF)

/* CAPS.HCSPARAMS2 fields */
#define XHCI_MAX_SP_HI(HCSPARAMS2) ((HCSPARAMS2 >> 25) & 0x1F)
#define XHCI_MAX_SP_LO(HCSPARAMS2) ((HCSPARAMS2 >> 31) & 0x1F)

/* PORTSC bits */
#define XHCI_PORTSC_CCS BIT(0)    /* Current connect status */
#define XHCI_PORTSC_PR  BIT(4)    /* Port reset */
#define XHCI_PORTSC_DR  BIT(30)   /* Device removable */

/* Registers */
#define XHCI_CAPS(BASE) PTR_OFFSET(BASE, 0)
#define XHCI_OPBASE(BASE, CAP_LEN) PTR_OFFSET(BASE, CAP_LEN)
#define XHCI_RTS(BASE, RTSOFF) PTR_OFFSET(BASE, RTSOFF)
#define XHCI_CMD_DB(BASE, DBOFF) PTR_OFFSET(BASE, DBOFF)

/* Runtime register offsets */
#define XHCI_RT_IMAN    0x20
#define XHCI_RT_IMOD    0x24
#define XHCI_RT_ERSTSZ  0x28
#define XHCI_RT_ERSTBA  0x30
#define XHCI_RT_ERDP    0x38

/* Support protocol cap fields */
#define XHCI_PROTO_ID(PROTO) (PROTO & 0xFF)
#define XHCI_PROTO_MINOR(PROTO) ((PROTO >> 16) & 0xFF)
#define XHCI_PROTO_MAJOR(PROTO) ((PROTO >> 24) & 0xFF)
#define XHCI_PROTO_NEXT(PROTO) ((PROTO >> 8) & 0xFF)
#define XHCI_PROTO_PORTOFF(PROTO2) (PROTO2 & 0xFF)
#define XHCI_PROTO_PORTCNT(PROTO2) ((PROTO2 >> 8) & 0xFF)

/* Extended cap IDs */
#define XHCI_ECAP_USBLEGSUP 1
#define XHCI_ECAP_PROTO     2

/* USBLEGSUP bits */
#define XHCI_BIOS_SEM   BIT(16)
#define XHCI_OS_SEM     BIT(24)

/* IMAN bits */
#define XHCI_IMAN_IP    BIT(0)
#define XHCI_IMAN_IE    BIT(1)

#endif  /* !_USB_XHCIREGS_H_ */

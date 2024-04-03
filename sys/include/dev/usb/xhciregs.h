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

#ifndef _USB_XHCIREGS_H_
#define _USB_XHCIREGS_H_

#include <sys/types.h>
#include <sys/cdefs.h>

/*
 * Host Controller Capability Registers
 *
 * See xHCI spec, section 5.3, table 5-9
 */
struct __packed xhci_caps {
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
struct __packed xhci_opregs {
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
#define USBCMD_RUN      __BIT(0)    /* Run/stop */
#define USBCMD_HCRST    __BIT(1)    /* xHC reset */

/* USBSTS bits */
#define USBSTS_HCH      __BIT(0)    /* HC halted */

/* CAPS.HCSPARAMS1 fields */
#define XHCI_MAXSLOTS(HCSPARAMS1) (HCSPARAMS1 & 0xFF)
#define XHCI_MAXPORTS(HCSPARAMS1) ((HCSPARAMS1 >> 24) & 0xFF)
#define XHCI_ECP(HCCPARAMS1) ((HCCPARAMS1 >> 16) & 0xFFFF)

/* CAPS.HCSPARAMS2 fields */
#define XHCI_MAX_SP_HI(HCSPARAMS2) ((HCSPARAMS2 >> 25) & 0x1F)
#define XHCI_MAX_SP_LO(HCSPARAMS2) ((HCSPARAMS2 >> 31) & 0x1F)

/* PORTSC bits */
#define XHCI_PORTSC_CCS __BIT(0)    /* Current connect status */
#define XHCI_PORTSC_PR  __BIT(4)    /* Port reset */

/* Registers */
#define XHCI_BASE_OFF(BASE, OFF) (void *)((uintptr_t)BASE + OFF)
#define XHCI_CAPS(BASE) XHCI_BASE_OFF(BASE, 0)
#define XHCI_OPBASE(BASE, CAP_LEN) XHCI_BASE_OFF(BASE, CAP_LEN)
#define XHCI_RTS(BASE, RTSOFF) XHCI_BASE_OFF(BASE, RTSOFF)
#define XHCI_CMD_DB(BASE, DBOFF) XHCI_BASE_OFF(BASE, DBOFF)

/* Support protocol cap fields */
#define XHCI_PROTO_ID(PROTO) (PROTO & 0xFF)
#define XHCI_PROTO_MINOR(PROTO) ((PROTO >> 16) & 0xFF)
#define XHCI_PROTO_MAJOR(PROTO) ((PROTO >> 24) & 0xFF)
#define XHCI_PROTO_NEXT(PROTO) ((PROTO >> 8) & 0xFF)
#define XHCI_PROTO_PORTOFF(PROTO2) (PROTO2 & 0xFF)
#define XHCI_PROTO_PORTCNT(PROTO2) ((PROTO2 >> 8) & 0xFF)

/* Extended cap IDs */
#define XHCI_ECAP_PROTO 2

#endif  /* !_USB_XHCIREGS_H_ */

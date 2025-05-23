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

#ifndef _USB_XHCIVAR_H_
#define _USB_XHCIVAR_H_

#include <sys/types.h>
#include <sys/types.h>
#include <sys/param.h>
#include <dev/usb/xhciregs.h>

#define XHCI_TIMEOUT 500    /* In ms */
#define XHCI_CMDRING_LEN 16
#define XHCI_EVRING_LEN 16
#define XHCI_TRB_SIZE 16   /* In bytes */
#define XHCI_MAX_PROTOS 4
#define XHCI_IMOD_DEFAULT 0

/* Quirks */
#define XHCI_QUIRK_HANDOFF BIT(0)

/*
 * USB proto (USB 2.0 or 3.0)
 */
struct xhci_proto {
    uint8_t major;      /* Revision major */
    uint8_t port_start; /* Port offset start */
    uint8_t port_count; /* Number of ports */
};

struct xhci_nop_trb {
    uint32_t reserved;
    uint32_t reserved1;
    uint32_t reserved2;
    uint8_t cycle : 1;
    uint16_t reserved3 : 9;
    uint8_t type : 6;
    uint16_t reserved4;
};

struct xhci_enableslot_trb {
    uint32_t reserved;
    uint32_t reserved1;
    uint32_t reserved2;
    uint8_t cycle : 1;
    uint16_t reserved3 : 9;
    uint8_t type : 6;
    uint8_t slot_type : 5;
    uint16_t reserved4 : 10;
};

/*
 * xHCI Transfer Request Block
 */
struct xhci_trb {
    union {
        struct xhci_nop_trb nop;
        struct xhci_enableslot_trb enableslot;
        struct {
            uint32_t dword0;
            uint32_t dword1;
            uint32_t dword2;
            uint32_t dword3;
        };
    };
};

/*
 * xHCI event ring segment
 *
 * See xHCI spec, section 6.5, table 6-40
 */
struct __packed xhci_evring_segment {
    uint64_t base;
    uint32_t size;
    uint32_t reserved;
};

/*
 * Host controller.
 */
struct xhci_hc {
    void *base;
    uintptr_t *dcbaap;
    uint32_t *cmdring;
    uint32_t *evring;
    uint8_t maxslots;
    uint8_t cr_cycle : 1;
    uint16_t quirks;
    size_t maxports;
    size_t protocnt;
    struct xhci_caps *caps;
    struct xhci_opregs *opregs;
    struct xhci_proto protos[XHCI_MAX_PROTOS];
};

/* TRB types */
#define XHCI_ENABLE_SLOT    9
#define XHCI_LINK           6

#endif  /* !_USB_XHCIVAR_H_ */

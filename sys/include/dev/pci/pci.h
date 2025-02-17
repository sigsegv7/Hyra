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

#ifndef _PCI_H_
#define _PCI_H_

#include <sys/types.h>
#include <sys/queue.h>

/* Lookup bits */
#define PCI_DEVICE_ID   BIT(0)
#define PCI_VENDOR_ID   BIT(1)
#define PCI_CLASS       BIT(2)
#define PCI_SUBCLASS    BIT(3)

typedef uint32_t pcireg_t;

/* For PCI lookups */
struct pci_lookup {
    uint16_t device_id;
    uint16_t vendor_id;
    uint8_t pci_class;
    uint8_t pci_subclass;
};

struct pci_device {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;

    uint16_t msix_capoff;
    uint16_t device_id;
    uint16_t vendor_id;
    uint8_t pci_class;
    uint8_t pci_subclass;
    uint8_t prog_if;
    uint8_t hdr_type;

    uint8_t pri_bus;
    uint8_t sec_bus;
    uint8_t sub_bus;

    uintptr_t bar[6];
    uint8_t irq_line;

    TAILQ_ENTRY(pci_device) link;
};

struct msi_intr {
    const char *name;
    void(*handler)(void *);
};

pcireg_t pci_readl(struct pci_device *dev, uint32_t offset);
struct pci_device *pci_get_device(struct pci_lookup lookup, uint16_t lookup_type);

int pci_map_bar(struct pci_device *dev, uint8_t barno, void **vap);
void pci_writel(struct pci_device *dev, uint32_t offset, pcireg_t val);

int pci_enable_msix(struct pci_device *dev, const struct msi_intr *intr);
void pci_msix_eoi(void);
int pci_init(void);

#endif  /* !_PCI_H_ */

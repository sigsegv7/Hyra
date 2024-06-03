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

#ifndef _DEV_PCI_H_
#define _DEV_PCI_H_

#include <sys/syscall.h>
#include <sys/queue.h>
#include <sys/cdefs.h>
#include <dev/pci/pciregs.h>
#include <vm/vm.h>

/* Lookup bits */
#define PCI_DEVICE_ID   __BIT(0)
#define PCI_VENDOR_ID   __BIT(1)
#define PCI_CLASS       __BIT(2)
#define PCI_SUBCLASS    __BIT(3)

/* Base address masks for BARs */
#define PCI_BAR_MEMMASK ~7
#define PCI_BAR_IOMASK  ~3

/* Macros to fetch base address from BAR */
#define PCI_BAR_MEMBASE(BAR) PHYS_TO_VIRT(BAR & PCI_BAR_MEMMASK)
#define PCI_BAR_IOBASE(BAR)  PHYS_TO_VIRT(BAR & PCI_BAR_IOMASK)

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

    uint16_t device_id;
    uint16_t vendor_id;
    uint8_t pci_class;
    uint8_t pci_subclass;
    uint8_t prog_if;
    uintptr_t bar[6];
    uint8_t irq_line;
    TAILQ_ENTRY(pci_device) link;
};

int pci_init(void);
uint32_t pci_readl(struct pci_device *dev, uint32_t offset);
void pci_writel(struct pci_device *dev, uint32_t offset, uint32_t val);
void pci_set_cmdreg(struct pci_device *dev, uint16_t bits);
int pci_map_bar(struct pci_device *dev, uint8_t bar, void **vap);
struct pci_device *pci_get_device(struct pci_lookup lookup, uint16_t lookup_type);

#endif  /* !_DEV_PCI_H_ */

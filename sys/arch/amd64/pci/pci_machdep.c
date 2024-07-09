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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <dev/pci/pci.h>
#include <dev/pci/pciregs.h>
#include <machine/pio.h>
#include <machine/bus.h>

/* Base address masks for BARs */
#define PCI_BAR_MEMMASK ~7

static inline uint32_t
pci_conf_addr(struct pci_device *dev, uint32_t offset)
{
    return BIT(31)             |
           (offset & ~3)       |
           (dev->func << 8)    |
           (dev->slot << 11)   |
           (dev->bus << 16);
}

/*
 * Convert a BAR number to BAR register offset.
 *
 * @dev: Device of BAR to check.
 * @bar: Bar number.
 */
static inline uint8_t
pci_get_barreg(struct pci_device *dev, uint8_t bar)
{
    switch (bar) {
    case 0: return PCIREG_BAR0;
    case 1: return PCIREG_BAR1;
    case 2: return PCIREG_BAR2;
    case 3: return PCIREG_BAR3;
    case 4: return PCIREG_BAR4;
    case 5: return PCIREG_BAR5;
    default: return 0;
    }
}

pcireg_t
pci_readl(struct pci_device *dev, uint32_t offset)
{
    uint32_t address;

    address = pci_conf_addr(dev, offset);
    outl(0xCF8, address);
    return inl(0xCFC) >> ((offset & 3) * 8);
}

void
pci_writel(struct pci_device *dev, uint32_t offset, pcireg_t val)
{
    uint32_t address;

    address = pci_conf_addr(dev, offset);
    outl(0xCF8, address);
    outl(0xCFC, val);
}

/*
 * Map a BAR into kernel memory.
 *
 * @dev: Device of BAR to map.
 * @barno: BAR number to map.
 * @vap: Resulting virtual address.
 */
int
pci_map_bar(struct pci_device *dev, uint8_t barno, void **vap)
{
    uint8_t barreg = pci_get_barreg(dev, barno);
    uintptr_t tmp, bar;
    uint32_t size;

    if (barreg == 0)
        return -EINVAL;

    /*
     * Get the length of the region this BAR covers by writing a
     * mask of 32 bits into the BAR register and seeing how many
     * bits are unset. We can use this to compute the size of the
     * region. We know that log2(len) bits must be unset.
     */
    tmp = pci_readl(dev, barreg);
    pci_writel(dev, barreg, 0xFFFFFFFF);
    size = pci_readl(dev, barreg);
    size = ~size + 1;

    /* Restore old value and map the BAR */
    pci_writel(dev, barreg, tmp);
    bar = dev->bar[barno] & PCI_BAR_MEMMASK;
    return bus_map(bar, size, 0, vap);
}

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

#include <sys/types.h>
#include <dev/pci/pci.h>

pcireg_t
pci_readl(struct pci_device *dev, uint32_t offset)
{
    /* TODO: STUB */
    return 0;
}

void
pci_writel(struct pci_device *dev, uint32_t offset, pcireg_t val)
{
    /* TODO: STUB */
    return;
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
    /* TODO: STUB */
    return 0;
}

void
pci_msix_eoi(void)
{
    return;
}

/*
 * Enable MSI-X for a device and allocate an
 * interrupt vector.
 *
 * @dev: Device to enable MSI-X for.
 * @intr: MSI-X interrupt descriptor.
 */
int
pci_enable_msix(struct pci_device *dev, const struct msi_intr *intr)
{
    /* TODO: STUB */
    return 0;
}

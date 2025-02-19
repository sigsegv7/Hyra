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
#include <sys/driver.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/mmio.h>
#include <dev/pci/pci.h>
#include <dev/ic/ahcivar.h>
#include <dev/ic/ahciregs.h>

#define pr_trace(fmt, ...) kprintf("ahci: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static struct pci_device *ahci_dev;

/*
 * Poll for at most 1 second, to see if GHC.HR is 0.
 *
 * @memspace: A pointer the HBA MMIO space.
 */
static int
poll_ghc_hr(struct hba_memspace *memspace)
{
    int count;

    for (count = 0; count < 10000000; count++) {
        if ((mmio_read32(&(memspace->ghc)) & 1) == 0) {
	    return 0;
        }
    }

    return 1;
}

static int
ahci_init(void)
{
    struct pci_lookup lookup;
    int status;
    struct ahci_hba hba;
    void *abar_vap = NULL;

    lookup.pci_class = 0x01;
    lookup.pci_subclass = 0x06;

    ahci_dev = pci_get_device(lookup, PCI_CLASS | PCI_SUBCLASS);
    if (ahci_dev == NULL) {
        return -ENODEV;
    }

    /*
     * The AHCI Host Bus Adapter (HBA) connects SATA
     * devices to the PCI bus, acting as an interface
     * between them.
     */
    pr_trace("Detected AHCI HBA (%x:%x.%x, slot=%d)\n",
        ahci_dev->bus, ahci_dev->device_id, ahci_dev->func,
        ahci_dev->slot);

    /*
     * Map the AHCI Base Address Register (ABAR) from the
     * ahci_dev struct, so that we can perform MMIO and then issue
     * a hard reset.
     */

    if ((status = pci_map_bar(ahci_dev, 5, &abar_vap)) != 0) {
        return status;
    }

    hba.io = (struct hba_memspace*) abar_vap;
    mmio_write32(&(hba.io->ghc), mmio_read32(&(hba.io->ghc)) | 1);

    if (poll_ghc_hr(hba.io) != 0) {
      return 1;
    }

    pr_trace("Successfully performed a hard reset.\n");
    return 0;
}

DRIVER_EXPORT(ahci_init);

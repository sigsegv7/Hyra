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
#include <sys/queue.h>
#include <sys/syslog.h>
#include <dev/pci/pci.h>
#include <dev/pci/pciregs.h>
#include <vm/dynalloc.h>
#include <lib/assert.h>

#define pr_trace(fmt, ...) kprintf("pci: " fmt, ##__VA_ARGS__)

static TAILQ_HEAD(, pci_device) device_list;

static bool
pci_dev_exists(uint8_t bus, uint8_t slot, uint8_t func)
{
    uint16_t vendor_id;
    struct pci_device dev_tmp;

    dev_tmp.bus = bus;
    dev_tmp.slot = slot;
    dev_tmp.func = func;
    vendor_id = pci_readl(&dev_tmp, PCIREG_VENDOR_ID);

    if (vendor_id == 0xFFFF) {
        return false;
    }

    return true;
}

/*
 * Sets other device information (device id, vendor id, etc)
 *
 * @dev: Device descriptor to set up.
 *
 * XXX: Expects device bus, slot and func to be set.
 */
static void
pci_set_device_info(struct pci_device *dev)
{
    uint32_t classrev;

    dev->vendor_id = pci_readl(dev, PCIREG_VENDOR_ID) & 0xFFFF;
    dev->device_id = pci_readl(dev, PCIREG_DEVICE_ID) & 0xFFFF;
    classrev = pci_readl(dev, PCIREG_CLASSREV);

    dev->pci_class = PCIREG_CLASS(classrev);
    dev->pci_subclass = PCIREG_SUBCLASS(classrev);
    dev->prog_if = PCIREG_PROGIF(classrev);

    dev->bar[0] = pci_readl(dev, PCIREG_BAR0);
    dev->bar[1] = pci_readl(dev, PCIREG_BAR1);
    dev->bar[2] = pci_readl(dev, PCIREG_BAR2);
    dev->bar[3] = pci_readl(dev, PCIREG_BAR3);
    dev->bar[4] = pci_readl(dev, PCIREG_BAR4);
    dev->bar[5] = pci_readl(dev, PCIREG_BAR5);

    dev->irq_line = pci_readl(dev, PCIREG_IRQLINE) & 0xFF;
}

/*
 * Attempt to register a device.
 *
 * @bus: Device bus number.
 * @slot: Device slot number.
 * @func: Device function number.
 *
 * This routine also checks if the device is present
 * and returns early if not.
 */
static void
pci_register_device(uint8_t bus, uint8_t slot, uint8_t func)
{
    struct pci_device *dev = NULL;

    if (!pci_dev_exists(bus, slot, func)) {
        return;
    }

    dev = dynalloc(sizeof(struct pci_device));
    __assert(dev != NULL);

    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;

    pci_set_device_info(dev);
    TAILQ_INSERT_TAIL(&device_list, dev, link);
}

static void
pci_scan_bus(uint8_t bus)
{
    for (int slot = 0; slot < 32; ++slot) {
        for (int func = 0; func < 8; ++func) {
            pci_register_device(bus, slot, func);
        }
    }
}

struct pci_device *
pci_get_device(struct pci_lookup lookup, uint16_t lookup_type)
{
    struct pci_device *dev;
    uint16_t lookup_matches = 0;

    TAILQ_FOREACH(dev, &device_list, link) {
        if (ISSET(lookup_type, PCI_DEVICE_ID)) {
            /* Check device ID */
            if (lookup.device_id == dev->device_id)
                lookup_matches |= PCI_DEVICE_ID;
        }

        if (ISSET(lookup_type, PCI_VENDOR_ID)) {
            /* Check vendor ID */
            if (lookup.vendor_id == dev->vendor_id)
                lookup_matches |= PCI_VENDOR_ID;
        }

        if (ISSET(lookup_type, PCI_CLASS)) {
            /* Check PCI class */
            if (lookup.pci_class == dev->pci_class)
                lookup_matches |= PCI_CLASS;
        }

        if (ISSET(lookup_type, PCI_SUBCLASS)) {
            /* Check PCI subclass */
            if (lookup.pci_subclass == dev->pci_subclass)
                lookup_matches |= PCI_SUBCLASS;
        }

        if (lookup_type == lookup_matches) {
            /* We found the device! */
            return dev;
        }

        lookup_matches = 0;
    }

    return NULL;
}

int
pci_init(void)
{
    TAILQ_INIT(&device_list);
    pr_trace("Scanning each bus...\n");

    for (uint16_t i = 0; i < 256; ++i) {
        pci_scan_bus(i);
    }

    return 0;
}

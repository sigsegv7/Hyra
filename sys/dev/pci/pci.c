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
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/errno.h>
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
 * Attempt to search for a capability within the device
 * capability list if it supports one.
 *
 * @dev: Target device.
 * @id: Requested capability ID.
 *
 * The offset is returned if found, otherwise 0.
 * A value less than zero is returned on error.
 */
static int
pci_get_cap(struct pci_device *dev, uint8_t id)
{
    uint16_t status;
    uint32_t cap;
    uint8_t curid;
    uint8_t cap_ptr;

    /* Does the device even support this? */
    status = pci_readl(dev, PCIREG_CMDSTATUS) >> 16;
    if (!ISSET(status, PCI_STATUS_CAPLIST)) {
        return -ENOTSUP;
    }

    cap_ptr = pci_readl(dev, PCIREG_CAPPTR) & 0xFF;

    /* Go through the capability list */
    while (cap_ptr != 0) {
        cap = pci_readl(dev, cap_ptr);
        curid = cap & 0xFF;

        if (curid == id) {
            return cap_ptr;
        }

        cap_ptr = (cap >> 8) & 0xFF;
    }

    return 0;
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
    uint32_t classrev, buses;
    int capoff;

    dev->vendor_id = pci_readl(dev, PCIREG_VENDOR_ID) & 0xFFFF;
    dev->device_id = pci_readl(dev, PCIREG_DEVICE_ID) & 0xFFFF;
    classrev = pci_readl(dev, PCIREG_CLASSREV);

    dev->pci_class = PCIREG_CLASS(classrev);
    dev->pci_subclass = PCIREG_SUBCLASS(classrev);
    dev->prog_if = PCIREG_PROGIF(classrev);
    dev->hdr_type = (uint8_t)pci_readl(dev, PCIREG_HDRTYPE);

    /* Set type-specific data */
    switch (dev->hdr_type & ~BIT(7)) {
    case PCI_HDRTYPE_NORMAL:
        dev->bar[0] = pci_readl(dev, PCIREG_BAR0);
        dev->bar[1] = pci_readl(dev, PCIREG_BAR1);
        dev->bar[2] = pci_readl(dev, PCIREG_BAR2);
        dev->bar[3] = pci_readl(dev, PCIREG_BAR3);
        dev->bar[4] = pci_readl(dev, PCIREG_BAR4);
        dev->bar[5] = pci_readl(dev, PCIREG_BAR5);

        dev->irq_line = pci_readl(dev, PCIREG_IRQLINE) & 0xFF;
        capoff = pci_get_cap(dev, PCI_CAP_MSIX);
        dev->msix_capoff = (capoff < 0) ? 0 : capoff;
        break;
    case PCI_HDRTYPE_BRIDGE:
        buses = pci_readl(dev, PCIREG_BUSES);
        dev->pri_bus = PCIREG_PRIBUS(buses);
        dev->sec_bus = PCIREG_SECBUS(buses);
        dev->sub_bus = PCIREG_SUBBUS(buses);
        break;
    default:
        break;
    }
}

static void
pci_scan_bus(uint8_t bus);

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

    /* Check if this is a valid bridge */
    if (
        (dev->hdr_type & ~BIT(7)) == PCI_HDRTYPE_BRIDGE &&
        dev->sec_bus > dev->bus &&
        dev->sub_bus >= dev->sec_bus
    ) {
        /* Scan all subordinate buses */
        for (uint8_t bus = dev->sec_bus; bus <= dev->sub_bus; ++bus) {
            pci_scan_bus(bus);
        }
    }

    TAILQ_INSERT_TAIL(&device_list, dev, link);
}

static void
pci_scan_bus(uint8_t bus)
{
    struct pci_device dev;

    dev.bus = bus;
    dev.func = 0;
    for (uint8_t slot = 0; slot < 32; ++slot) {
        dev.slot = slot;

        /* Skip nonexistent device */
        if ((uint16_t)pci_readl(&dev, PCIREG_VENDOR_ID) == 0xFFFF) {
            continue;
        }

        /* Register single-function device */
        if (!(pci_readl(&dev, PCIREG_HDRTYPE) & BIT(7))) {
            pci_register_device(bus, slot, 0);
            continue;
        }

        /* Register all functions */
        for (uint8_t func = 0; func < 8; ++func) {
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
    size_t ndev;
    TAILQ_INIT(&device_list);

    /* Recursively scan bus 0 */
    pci_scan_bus(0);
    ndev = TAILQ_NELEM(&device_list);
    pr_trace("detected %d devices at pci*\n", ndev);
    return 0;
}

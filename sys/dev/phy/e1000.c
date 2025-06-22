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
#include <dev/phy/e1000regs.h>
#include <dev/pci/pci.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("e1000: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define E1000_VENDOR 0x8086
#define E1000_DEVICE 0x100E

static struct pci_device *e1000;

struct e1000_nic {
    void *vap;
    uint8_t has_eeprom : 1;
    uint16_t eeprom_size;
};

/*
 * Query information about any EEPROMs for diagnostic
 * purposes.
 *
 * TODO: Some wacky older chips don't show their presence
 *       too easily, we could fallback to microwire / SPI
 *       bit banging to see if it responds to us manually
 *       clocking a dummy read operation in.
 */
static void
eeprom_query(struct e1000_nic *np)
{
    uint16_t size_bits = 1024;
    uint32_t eecd, *eecd_p;
    const char *typestr = "microwire";

    eecd_p = PTR_OFFSET(np->vap, E1000_EECD);

    /*
     * First we should check if there is an EEPROM
     * on-board as if not, there is nothing we can do
     * here.
     */
    eecd = mmio_read32(eecd_p);
    if (!ISSET(eecd, E1000_EECD_PRES)) {
        return;
    }

    np->has_eeprom = 1;
    if (ISSET(eecd, E1000_EECD_TYPE)) {
        typestr = "SPI";
    }
    if (ISSET(eecd, E1000_EECD_SIZE)) {
        size_bits = 4096;
    }

    np->eeprom_size = size_bits;
    pr_trace("%d-bit %s EEPROM detected\n", size_bits, typestr);
}

/*
 * Initialize an E1000(e) chip
 */
static int
e1000_chip_init(struct e1000_nic *np)
{
    eeprom_query(np);
    return 0;
}

/*
 * Enables PCI specific bits like bus mastering (for DMA)
 * as well as MMIO.
 */
static void
e1000_init_pci(void)
{
    uint32_t tmp;

    tmp = pci_readl(e1000, PCIREG_CMDSTATUS);
    tmp |= (PCI_BUS_MASTERING | PCI_MEM_SPACE);
    pci_writel(e1000, PCIREG_CMDSTATUS, tmp);
}

static int
e1000_init(void)
{
    struct pci_lookup lookup;
    struct e1000_nic nic;
    int status;

    lookup.vendor_id = E1000_VENDOR;
    lookup.device_id = E1000_DEVICE;
    e1000 = pci_get_device(lookup, PCI_DEVICE_ID | PCI_VENDOR_ID);
    if (e1000 == NULL) {
        return -ENODEV;
    }

    memset(&nic, 0, sizeof(nic));
    pr_trace("e1000 at pci%d:%x.%x.%d\n",
        e1000->bus, e1000->device_id, e1000->func,
        e1000->slot);

    if ((status = pci_map_bar(e1000, 0, &nic.vap)) != 0) {
        pr_error("failed to map BAR0\n");
        return status;
    }

    e1000_init_pci();
    e1000_chip_init(&nic);
    return 0;
}

DRIVER_EXPORT(e1000_init);

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

/*
 * This driver is the product of reverse engineering
 * work done by Ian Marco Moffett and the OSMORA team.
 *
 * Please refer to share/docs/hw/et131x.txt
 */

#include <sys/types.h>
#include <sys/driver.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/mmio.h>
#include <dev/pci/pci.h>
#include <dev/pci/pciregs.h>
#include <dev/phy/et131xregs.h>
#include <dev/timer.h>

#define VENDOR_ID 0x11C1 /* Agere */
#define DEVICE_ID 0xED00

#define pr_trace(fmt, ...) kprintf("et131x: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static struct pci_device *dev;
static struct timer tmr;

/*
 * Software reset the ET131X
 *
 * @io: Register space
 */
static void
et131x_soft_reset(struct et131x_iospace *io)
{
    uint32_t tmp;

    tmp = (
        MAC_CFG1_RESET_TXMC   |
        MAC_CFG1_RESET_RXMC   |
        MAC_CFG1_RESET_TXFUNC |
        MAC_CFG1_RESET_RXFUNC |
        MAC_CFG1_SOFTRST      |
        MAC_CFG1_SIMRST
    );

    /*
     * Reset the MAC core, bring it down. After that,
     * we perform a global reset to bring the whole
     * chip down.
     */
    mmio_write32(&io->mac.cfg1, tmp);
    mmio_write32(&io->global.sw_reset, GBL_RESET_ALL);

    /*
     * Reset the MAC again for good measure, but
     * this time a little softer. We already slammed
     * the poor thing.
     */
    tmp &= ~(MAC_CFG1_SOFTRST | MAC_CFG1_SIMRST);
    mmio_write32(&io->mac.cfg1, tmp);
    mmio_write32(&io->mac.cfg1, 0);
}

/*
 * Write to the PHY through MII
 *
 * @io: Register space
 * @addr: PHY address
 * @reg: PHY register
 * @v: Value to write
 */
static int
et131x_mii_write(struct et131x_iospace *io, uint8_t addr, uint8_t reg, uint16_t v)
{
    uint16_t mii_addr;
    uint32_t tmp, mgmt_addr_old;
    uint32_t mgmt_cmd_old;
    uint8_t ndelay = 0;
    int retval = 0;

    /* Save MII management regs state */
    mgmt_cmd_old = mmio_read32(&io->mac.mii_mgmt_cmd);
    mgmt_addr_old = mmio_read32(&io->mac.mii_mgmt_addr);
    mii_addr = MAC_MII_ADDR(addr, reg);

    /*
     * Stop any transactions that are currently
     * happening on the MDIO bus and prepare the
     * write.
     */
    mmio_write32(&io->mac.mii_mgmt_cmd, 0);
    mmio_write32(&io->mac.mii_mgmt_addr, mii_addr);
    mmio_write32(&io->mac.mii_mgmt_ctrl, v);

    for (;;) {
        tmr.usleep(50);
        ++ndelay;

        tmp = mmio_read32(&io->mac.mii_mgmt_indicator);
        if (!ISSET(tmp, MAC_MGMT_BUSY))
            break;
        if (ndelay >= 50)
            break;
    }

    if (ndelay >= 50) {
        pr_error("could not write PHY reg %x (status=%x)\n", reg, tmp);
        retval = -EIO;
        goto done;
    }

done:
    /* Stop operations and restore state */
    mmio_write32(&io->mac.mii_mgmt_cmd, 0);
    mmio_write32(&io->mac.mii_mgmt_addr, mgmt_addr_old);
    mmio_write32(&io->mac.mii_mgmt_cmd, mgmt_cmd_old);
    return retval;
}

/*
 * Initialize PCI related things for the
 * chip.
 */
static void
et131x_init_pci(void)
{
    uint32_t tmp;

    /* Enable bus mastering and MMIO */
    tmp = pci_readl(dev, PCIREG_CMDSTATUS);
    tmp |= (PCI_BUS_MASTERING | PCI_MEM_SPACE);
    pci_writel(dev, PCIREG_CMDSTATUS, tmp);
}

/*
 * Blink both LEDs of the card
 *
 * @io: Register space
 * @count: Number of times to blink
 * @delay: Millisecond delay between blinks
 */
static void
et131x_blink(struct et131x_iospace *io, uint32_t count, uint16_t delay)
{
    uint16_t on_val;

    on_val =  (LED_ON << LED_LINK_SHIFT);
    on_val |= (LED_ON << LED_TXRX_SHIFT);
    for (uint32_t i = 0; i < count; ++i) {
        et131x_mii_write(io, 0, PHY_LED2, on_val);
        tmr.msleep(delay);
        et131x_mii_write(io, 0, PHY_LED2, LED_ALL_OFF);
        tmr.msleep(delay);
    }
}

static int
et131x_init(void)
{
    struct pci_lookup lookup;
    struct et131x_iospace *bar;
    int error;

    lookup.vendor_id = VENDOR_ID;
    lookup.device_id = DEVICE_ID;
    dev = pci_get_device(lookup, PCI_VENDOR_ID | PCI_DEVICE_ID);
    if (dev == NULL) {
        return -ENODEV;
    }

    pr_trace("Agere ET1310 Ethernet ctl <phy? at pci%d:%x.%x.%d>\n",
        dev->bus, dev->device_id, dev->func,
        dev->slot);

    /* Try to request a general purpose timer */
    if (req_timer(TIMER_GP, &tmr) != TMRR_SUCCESS) {
        pr_error("failed to fetch general purpose timer\n");
        return -ENODEV;
    }

    /* Ensure it has get_time_usec() */
    if (tmr.usleep == NULL) {
        pr_error("general purpose timer has no usleep()\n");
        return -ENODEV;
    }

    if ((error = pci_map_bar(dev, 0, (void *)&bar)) != 0) {
        return error;
    }

    et131x_init_pci();
    et131x_soft_reset(bar);
    et131x_blink(bar, 4, 150);
    return 0;
}

DRIVER_DEFER(et131x_init, "et131x");

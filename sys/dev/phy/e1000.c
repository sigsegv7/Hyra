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
#include <dev/pci/pciregs.h>
#include <dev/timer.h>
#include <net/if_var.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("e1000: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define E1000_VENDOR 0x8086
#define E1000_DEVICE 0x100E
#define E1000_TIMEOUT 500       /* In msec */

static struct timer tmr;
static struct pci_device *e1000;
static struct netif netif;

struct e1000_nic {
    void *vap;
    uint8_t has_eeprom : 1;
    uint16_t eeprom_size;
    uint16_t io_port;
};

static int
e1000_poll_reg(volatile uint32_t *reg, uint32_t bits, bool pollset)
{
    size_t usec_start, usec;
    size_t elapsed_msec;
    uint32_t val;
    bool tmp;

    usec_start = tmr.get_time_usec();

    for (;;) {
        val = mmio_read32(reg);
        tmp = (pollset) ? ISSET(val, bits) : !ISSET(val, bits);

        usec = tmr.get_time_usec();
        elapsed_msec = (usec - usec_start) / 1000;

        /* If tmp is set, the register updated in time */
        if (tmp) {
            break;
        }

        /* Exit with an error if we timeout */
        if (elapsed_msec > E1000_TIMEOUT) {
            return -ETIME;
        }
    }

    return 0;
}

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
 * If there is no EEPROM, we can still read
 * the MAC address through the Receive address
 * registers
 *
 * XXX: This is typically only used as a fallback.
 *
 * Returns a less than zero value if an ethernet
 * address is not found, which would be kind of
 * not good.
 *
 * @np: NIC descriptor
 * @addr: Pointer to MAC address data
 */
static int
e1000_read_recvaddr(struct e1000_nic *np, struct netif_addr *addr)
{
    const uint32_t RECVADDR_OFF = 0x5400;
    uint32_t tmp;
    uint32_t *dword_p;

    dword_p = PTR_OFFSET(np->vap, RECVADDR_OFF);

    if (dword_p[0] == 0) {
        pr_error("bad hwaddr in recvaddr\n");
        return -ENOTSUP;
    }

    /* DWORD 0 */
    tmp = mmio_read32(&dword_p[0]);
    addr->data[0] = tmp & 0xFF;
    addr->data[1] = (tmp >> 8) & 0xFF;
    addr->data[2] = (tmp >> 16) & 0xFF;
    addr->data[3] = (tmp >> 24) & 0xFF;

    /* DWORD 1 */
    tmp = mmio_read32(&dword_p[1]);
    addr->data[4] = tmp & 0xFF;
    addr->data[5] = (tmp >> 8) & 0xFF;
    return 0;
}

/*
 * Read 16-bytes from the NIC's on-board EEPROM.
 *
 * XXX: This should only be used if the caller is
 *      certain that the NIC has an EEPROM
 *
 * @addr: EEPROM address to read from
 *
 * A returned value of 0xFFFF should be seen as invalid.
 */
static uint16_t
eeprom_readw(struct e1000_nic *np, uint8_t addr)
{
    uint32_t eerd, *eerd_p;
    int error;

    if (!np->has_eeprom) {
        pr_error("e1000_read_eeprom: EEPROM not present\n");
        return 0xFFFF;
    }

    eerd_p = PTR_OFFSET(np->vap, E1000_EERD);
    eerd = (addr << 8) | E1000_EERD_START;
    mmio_write32(eerd_p, eerd);

    error = e1000_poll_reg(eerd_p, E1000_EERD_DONE, true);
    if (error < 0) {
        pr_error("e1000_read_eeprom: timeout\n");
        return 0xFFFF;
    }

    eerd = mmio_read32(eerd_p);
    return (eerd >> 16) & 0xFFFF;
}

/*
 * Read the MAC address from the NICs EEPROM.
 *
 * XXX: This should usually work, however if the NIC does
 *      not have an on-board EEPROM, this will fail. In such
 *      cases, e1000_read_recvaddr() can be called instead.
 *
 * @np: NIC descriptor
 * @addr: Pointer to MAC address data
 */
static int
e1000_read_macaddr(struct e1000_nic *np, struct netif_addr *addr)
{
    uint16_t eeprom_word;

    if (!np->has_eeprom) {
        pr_trace("EEPROM not present, trying recvaddr\n");
        return e1000_read_recvaddr(np, addr);
    }

    /* Word 0 */
    eeprom_word = eeprom_readw(np, E1000_HWADDR0);
    addr->data[0] = (eeprom_word & 0xFF);
    addr->data[1] = (eeprom_word >> 8) & 0xFF;

    /* Word 1 */
    eeprom_word = eeprom_readw(np, E1000_HWADDR1);
    addr->data[2] = (eeprom_word & 0xFF);
    addr->data[3] = (eeprom_word >> 8) & 0xFF;

    /* Word 2 */
    eeprom_word = eeprom_readw(np, E1000_HWADDR2);
    addr->data[4] = (eeprom_word & 0xFF);
    addr->data[5] = (eeprom_word >> 8) & 0xFF;
    return 0;
}

/*
 * Reset the entire E1000
 */
static int
e1000_reset(struct e1000_nic *np)
{
    uint32_t ctl, *ctl_p;
    int error;

    ctl_p = PTR_OFFSET(np->vap, E1000_CTL);
    ctl = mmio_read32(&ctl_p);
    ctl |= E1000_CTL_RST;
    mmio_write32(&ctl_p, ctl);

    error = e1000_poll_reg(ctl_p, E1000_CTL_RST, false);
    if (error < 0) {
        pr_error("reset timeout\n");
        return error;
    }

    return 0;
}

/*
 * Initialize an E1000(e) chip
 */
static int
e1000_chip_init(struct e1000_nic *np)
{
    struct netif_addr *addr = &netif.addr;
    int error;

    /*
     * To ensure that BIOS/UEFI or whatever firmware got us
     * here didn't fuck anything up in the process or at the
     * very least, put the controller in a seemingly alright
     * state that gives us a suprise screwing in the future,
     * we'll reset everything to its default startup state.
     *
     * Better safe than sorry...
     */
    if ((error = e1000_reset(np)) < 0) {
        return error;
    }

    eeprom_query(np);
    if ((error = e1000_read_macaddr(np, addr)) < 0) {
        return error;
    }

    pr_trace("MAC address: %x:%x:%x:%x:%x:%x\n",
        (uint64_t)addr->data[0], (uint64_t)addr->data[1],
        (uint64_t)addr->data[2], (uint64_t)addr->data[3],
        (uint64_t)addr->data[4], (uint64_t)addr->data[5]);

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

    /* Get a GP timer */
    if (req_timer(TIMER_GP, &tmr) != TMRR_SUCCESS) {
        pr_error("failed to fetch general purpose timer\n");
        return -ENODEV;
    }

    /* We need msleep() */
    if (tmr.msleep == NULL) {
        pr_error("general purpose timer has no msleep()\n");
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

DRIVER_EXPORT(e1000_init, "e1000");

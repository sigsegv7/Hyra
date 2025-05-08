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
#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/mmio.h>
#include <dev/pci/pci.h>
#include <dev/timer.h>
#include <dev/ic/ahcivar.h>
#include <dev/ic/ahciregs.h>
#include <vm/dynalloc.h>
#include <vm/physmem.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("ahci: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static struct hba_device *devs;
static struct pci_device *ahci_dev;
static struct timer tmr;

/*
 * Poll register to have 'bits' set/unset.
 *
 * @reg: Register to poll.
 * @bits: Bits to be checked.
 * @pollset: True to poll as set.
 */
static int
ahci_poll_reg(volatile uint32_t *reg, uint32_t bits, bool pollset)
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
        if (elapsed_msec > AHCI_TIMEOUT) {
            return -ETIME;
        }
    }

    return 0;
}

static int
ahci_hba_reset(struct ahci_hba *hba)
{
    struct hba_memspace *abar = hba->io;
    uint32_t tmp;
    int error;

    /*
     * Begin the actual reset process, results in all
     * HBA state and ports being reset.
     *
     * XXX: All ports will need be to brought back up
     *      through a COMRESET...
     */
    tmp = mmio_read32(&abar->ghc);
    tmp |= AHCI_GHC_HR;
    mmio_write32(&abar->ghc, tmp);

    /*
     * This should usually work with no problem but we cannot
     * guarantee anything. Especially if it comes to quirky
     * hardware. The GHC.HR bit should be flipped back by the
     * HBA once the reset is complete.
     */
    error = ahci_poll_reg(&abar->ghc, AHCI_GHC_HR, false);
    if (error < 0) {
        pr_error("HBA reset failed\n");
        return error;
    }

    return 0;
}

/*
 * Stop an HBA port's command list and FIS
 * engine.
 */
static int
hba_port_stop(struct hba_port *port)
{
    uint32_t cmd, tmp;

    /* Stop the port */
    cmd = mmio_read32(&port->cmd);
    cmd &= ~(AHCI_PXCMD_ST | AHCI_PXCMD_FRE);
    mmio_write32(&port->cmd, cmd);

    /*
     * The spec states that once the port is stopped,
     * PxCMD.CR and PxCMD.FR become unset
     */
    tmp = AHCI_PXCMD_FR | AHCI_PXCMD_CR;
    if (ahci_poll_reg(&port->cmd, tmp, false) < 0) {
        return -EAGAIN;
    }

    return 0;
}

/*
 * Bring up an HBA port's command list
 * and FIS engine.
 */
static int
hba_port_start(struct hba_port *port)
{
    uint32_t cmd, tmp;

    /* Bring up the port */
    cmd = mmio_read32(&port->cmd);
    cmd |= AHCI_PXCMD_ST | AHCI_PXCMD_FRE;
    mmio_write32(&port->cmd, cmd);

    tmp = AHCI_PXCMD_FR | AHCI_PXCMD_CR;
    if (ahci_poll_reg(&port->cmd, tmp, true) < 0) {
        return -EAGAIN;
    }

    return 0;
}

/*
 * Initialize a drive on an HBA port
 *
 * @hba: HBA descriptor
 * @portno: Port number
 */
static int
ahci_init_port(struct ahci_hba *hba, uint32_t portno)
{
    struct hba_memspace *abar = hba->io;
    struct hba_port *port;
    struct hba_device *dp;
    size_t clen, pagesz;
    uint32_t lo, hi;
    paddr_t fra, cmdlist, tmp;
    int error;

    pr_trace("found device @ port %d\n", portno);
    pagesz = DEFAULT_PAGESIZE;
    port = &abar->ports[portno];

    if ((error = hba_port_stop(port)) < 0) {
        pr_trace("failed to stop port %d\n", portno);
        return error;
    }

    dp = &devs[portno];
    dp->io = port;
    dp->hba = hba;
    dp->dev = portno;

    /* Allocate a command list */
    clen = ALIGN_UP(hba->nslots * AHCI_CMDENTRY_SIZE, pagesz);
    clen /= pagesz;
    cmdlist = vm_alloc_frame(clen);
    if (cmdlist == 0) {
        pr_trace("failed to alloc command list\n");
        return -ENOMEM;
    }

    /* Allocate FIS receive area */
    dp->cmdlist = PHYS_TO_VIRT(cmdlist);
    fra = vm_alloc_frame(1);
    if (fra == 0) {
        pr_trace("failed to allocate FIS receive area\n");
        vm_free_frame(cmdlist, clen);
        return -ENOMEM;
    }

    dp->fra = PHYS_TO_VIRT(fra);
    memset(dp->cmdlist, 0, clen * pagesz);
    memset(dp->fra, 0, pagesz);

    /* Write the command list */
    lo = cmdlist & 0xFFFFFFFF;
    hi = cmdlist >> 32;
    mmio_write32(&port->clb, lo);
    mmio_write32(&port->clbu, hi);

    /* Write the FIS receive area */
    lo = fra & 0xFFFFFFFF;
    hi = fra >> 32;
    mmio_write32(&port->fb, lo);
    mmio_write32(&port->fbu, hi);

    /* Each command header has a H2D FIS area */
    for (int i = 0; i < hba->nslots; ++i) {
        tmp = vm_alloc_frame(1);
        dp->cmdlist[i].prdtl = 1;
        dp->cmdlist[i].ctba = tmp;
        memset(PHYS_TO_VIRT(tmp), 0, pagesz);
    }

    if ((error = hba_port_start(port)) < 0) {
        for (int i = 0; i < hba->nslots; ++i) {
            vm_free_frame(dp->cmdlist[i].ctba, 1);
        }
        vm_free_frame(cmdlist, clen);
        vm_free_frame(fra, 1);
        pr_trace("failed to start port %d\n", portno);
        return error;
    }
    return 0;
}

/*
 * Scan the HBA for implemented ports
 */
static int
ahci_hba_scan(struct ahci_hba *hba)
{
    struct hba_memspace *abar = hba->io;
    uint32_t pi, i = 0;
    size_t len;

    len = hba->nports * sizeof(struct hba_device);
    if ((devs = dynalloc(len)) == NULL) {
        pr_trace("failed to allocate dev descriptors\n");
        return -ENOMEM;
    }

    memset(devs, 0, len);
    pi = mmio_read32(&abar->pi);
    while (pi != 0) {
        if ((pi & 1) != 0) {
            ahci_init_port(hba, i);
        }

        ++i;
        pi >>= 1;
    }

    return 0;
}

static int
ahci_hba_init(struct ahci_hba *hba)
{
    struct hba_memspace *abar = hba->io;
    int error;
    uint32_t tmp;
    uint32_t cap, pi;

    /*
     * God knows what state the HBA is in by the time
     * BIOS/UEFI passes control to us... Because of this,
     * we need to *reset* everything to ensure it is
     * as we expect.
     */
    if ((error = ahci_hba_reset(hba)) < 0) {
        return error;
    }

    pr_trace("successfully performed a hard reset\n");
    cap = mmio_read32(&abar->cap);
    hba->maxports = AHCI_CAP_NP(cap);
    hba->nslots = AHCI_CAP_NCS(cap);
    hba->ems = AHCI_CAP_EMS(cap);
    hba->sal = AHCI_CAP_SAL(cap);

    /*
     * The HBA provides backwards compatibility with
     * legacy ATA mechanisms (e.g., SFF-8038i), therefore
     * in order for this driver to work we need to ensure
     * the HBA is AHCI aware.
     */
    tmp = mmio_read32(&abar->ghc);
    tmp |= AHCI_GHC_AE;
    mmio_write32(&abar->ghc, tmp);

    /*
     * CAP.NCS reports the maximum number of ports the
     * HBA silicon supports but a lot of hardware will
     * not implement the full number of ports supported.
     *
     * the `PI' register is a bit-significant register
     * used to determine which ports are implemented,
     * therefore we can just count how many bits are
     * set in this register and that would be how many
     * ports are implemented total.
     */
    pi = mmio_read32(&abar->pi);
    hba->nports = popcnt(pi);
    pr_trace("hba implements %d port(s)\n", hba->nports);

    if ((error = ahci_hba_scan(hba)) != 0) {
        return error;
    }

    return 0;
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
    pr_trace("IDE storage ctrl <hba? at pci%d:%x.%x.%d>\n",
        ahci_dev->bus, ahci_dev->device_id, ahci_dev->func,
        ahci_dev->slot);

    /* Try to request a general purpose timer */
    if (req_timer(TIMER_GP, &tmr) != TMRR_SUCCESS) {
        pr_error("failed to fetch general purpose timer\n");
        return -ENODEV;
    }

    /* Ensure it has get_time_usec() */
    if (tmr.get_time_usec == NULL) {
        pr_error("general purpose timer has no get_time_usec()\n");
        return -ENODEV;
    }

    /* We also need msleep() */
    if (tmr.msleep == NULL) {
        pr_error("general purpose timer has no msleep()\n");
        return -ENODEV;
    }

    /*
     * Map the AHCI Base Address Register (ABAR) from the
     * ahci_dev struct, so that we can perform MMIO and then issue
     * a hard reset.
     */
    if ((status = pci_map_bar(ahci_dev, 5, &abar_vap)) != 0) {
        return status;
    }

    hba.io = (struct hba_memspace*)abar_vap;
    ahci_hba_init(&hba);
    return 0;
}

DRIVER_EXPORT(ahci_init);

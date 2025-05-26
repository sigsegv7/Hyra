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
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/sio.h>
#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/mmio.h>
#include <dev/pci/pci.h>
#include <dev/pci/pciregs.h>
#include <dev/timer.h>
#include <dev/ic/ahcivar.h>
#include <dev/ic/ahciregs.h>
#include <dev/dcdr/cache.h>
#include <fs/devfs.h>
#include <fs/ctlfs.h>
#include <vm/dynalloc.h>
#include <vm/physmem.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("ahci: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static uint32_t devs_max = 0;
static struct bdevsw ahci_bdevsw;
static struct hba_device *devs;
static struct pci_device *ahci_dev;
static struct timer tmr;
static struct ahci_hba g_hba;

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

/*
 * Allocate a command slot for a port on
 * the HBA.
 */
static int
ahci_alloc_cmdslot(struct ahci_hba *hba, struct hba_port *port)
{
    uint32_t slotlist;

    slotlist = mmio_read32(&port->ci);
    slotlist |= mmio_read32(&port->sact);

    for (int i = 0; i < hba->nslots; ++i) {
        if (!ISSET(slotlist, i)) {
            return i;
        }
    }

    return -EAGAIN;
}

/*
 * Get the command list base.
 */
static paddr_t
ahci_cmdbase(struct hba_port *port)
{
    paddr_t basel, baseh, base;

    basel = mmio_read32(&port->clb);
    baseh = mmio_read32(&port->clbu);
    base = COMBINE32(baseh, basel);
    return base;
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
 * Dump identify structure for debugging
 * purposes.
 */
static void
ahci_dump_identity(struct ata_identity *identity)
{
    char serial_number[20];
    char model_number[40];
    char tmp;

    memcpy(serial_number, identity->serial_number, sizeof(serial_number));
    memcpy(model_number, identity->model_number, sizeof(model_number));

    serial_number[sizeof(serial_number) - 1] = '\0';
    model_number[sizeof(model_number) - 1] = '\0';

    /* Fixup endianess for serial number */
    for (size_t i = 0; i < sizeof(serial_number); i += 2) {
        tmp = serial_number[i];
        serial_number[i] = serial_number[i + 1];
        serial_number[i + 1] = tmp;
    }

    /* Fixup endianess for model number */
    for (size_t i = 0; i < sizeof(model_number); i += 2) {
        tmp = model_number[i];
        model_number[i] = model_number[i + 1];
        model_number[i + 1] = tmp;
    }

    pr_trace("model number: %s\n", model_number);
}

/*
 * Stop an HBA port's command list and FIS
 * engine.
 */
static int
hba_port_stop(struct hba_port *port)
{
    const uint32_t RUN_MASK = (AHCI_PXCMD_FR | AHCI_PXCMD_CR);
    uint32_t cmd, tmp;

    /* Ensure the port is running */
    cmd = mmio_read32(&port->cmd);
    if (!ISSET(cmd, RUN_MASK)) {
        return 0;
    }

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
    const uint32_t RUN_MASK = (AHCI_PXCMD_FR | AHCI_PXCMD_CR);
    uint32_t cmd, tmp;

    /* Ensure the port is not running */
    cmd = mmio_read32(&port->cmd);
    if (ISSET(cmd, RUN_MASK)) {
        return 0;
    }

    /* Bring up the port */
    cmd |= AHCI_PXCMD_ST | AHCI_PXCMD_FRE;
    mmio_write32(&port->cmd, cmd);

    tmp = AHCI_PXCMD_FR | AHCI_PXCMD_CR;
    if (ahci_poll_reg(&port->cmd, tmp, true) < 0) {
        return -EAGAIN;
    }

    return 0;
}

/*
 * Check for interface errors, returns
 * 0 on success (i.e., no errors), otherwise
 * the "ERR" word of PxSERR.
 */
static int
hba_port_chkerr(struct hba_port *port)
{
    uint32_t serr;
    uint16_t err;
    uint8_t critical = 0;

    serr = mmio_read32(&port->serr);
    err = serr & 0xFFFF;
    if (err == 0) {
        return 0;
    }

    if (ISSET(err, AHCI_SERR_I)) {
        pr_error("recovered data integrity error\n");
    }
    if (ISSET(err, AHCI_SERR_M)) {
        pr_error("recovered comms error\n");
    }
    if (ISSET(err, AHCI_SERR_T)) {
        pr_error("transient data integrity error\n");
    }
    if (ISSET(err, AHCI_SERR_C)) {
        pr_error("persistent comms error\n");
        critical = 1;
    }
    if (ISSET(err, AHCI_SERR_P)) {
        pr_error("protocol error\n");
        critical = 1;
    }
    if (ISSET(err, AHCI_SERR_E)) {
        pr_error("internal hba error\n");
        critical = 1;
    }
    if (critical) {
        pr_error("CRITICAL - DISABLING PORT **\n");
        hba_port_stop(port);
        return err;
    }

    mmio_write32(&port->serr, 0xFFFFFFFF);
    return err;

}

/*
 * Reset a port on the HBA
 *
 * XXX: This function stops the port once the
 *      COMRESET is complete.
 */
static int
hba_port_reset(struct ahci_hba *hba, struct hba_port *port)
{
    uint32_t sctl, ssts;
    uint8_t det, ipm;
    int error;

    /*
     * The port must not be in an idle state when a
     * COMRESET is sent over the interface as some
     * chipsets do not know how to handle this...
     *
     * After bringing up the port, send a COMRESET
     * over the interface for roughly ~2ms.
     */
    hba_port_start(port);
    sctl = mmio_read32(&port->sctl);
    sctl = (sctl & ~0x0F) | AHCI_DET_COMRESET;
    mmio_write32(&port->sctl, sctl);

    /*
     * Wait for the link to become reestablished
     * between the port and the HBA.
     */
    tmr.msleep(300);
    sctl &= ~AHCI_DET_COMRESET;
    mmio_write32(&port->sctl,  sctl);

    /*
     * Now we'll need to grab some power management
     * and detection flags as the port must have
     * a device present along with an active
     * interface.
     */
    ssts = mmio_read32(&port->ssts);
    det = AHCI_PXSCTL_DET(ssts);
    ipm = AHCI_PXSSTS_IPM(ssts);

    /* If there is no device, fake success */
    if (det == AHCI_DET_NULL) {
        return 0;
    }

    if (det != AHCI_DET_COMM) {
        pr_trace("failed to establish link\n");
        return -EAGAIN;
    }

    if (ipm != AHCI_IPM_ACTIVE) {
        pr_trace("device interface not active\n");
        return -EAGAIN;
    }

    if ((error = hba_port_stop(port)) < 0) {
        pr_trace("failed to stop port\n");
        return error;
    }

    return 0;
}

static int
ahci_submit_cmd(struct ahci_hba *hba, struct hba_port *port, uint8_t slot)
{
    const uint32_t BUSY_BITS = (AHCI_PXTFD_BSY | AHCI_PXTFD_DRQ);
    const uint8_t MAX_ATTEMPTS = 3;
    uint32_t ci;
    uint8_t attempts = 0;
    int status = 0;

    /*
     * Spin on `TFD.BSY` and `TFD.DRQ` to ensure
     * that the port is not busy before we send
     * any commands.
     */
    if (ahci_poll_reg(&port->tfd, BUSY_BITS, false) < 0) {
        pr_trace("cmd failed, port busy (slot=%d)\n", slot);
        return -EBUSY;
    }

    /*
     * Submit and wait for completion, this may take
     * a bit so give it several attempts.
     */
    ci = mmio_read32(&port->ci);
    mmio_write32(&port->ci, ci | BIT(slot));
    while ((attempts++) < MAX_ATTEMPTS) {
        status = ahci_poll_reg(&port->ci, BIT(slot), false);
        if (status == 0) {
            break;
        }
    }
    if (status != 0) {
        return status;
    }

    return hba_port_chkerr(port);
}

/*
 * Send an ATA IDENTIFY command to a
 * SATA device.
 */
static int
ahci_identify(struct ahci_hba *hba, struct hba_port *port)
{
    paddr_t base, buf;
    struct ahci_cmd_hdr *cmdhdr;
    struct ahci_cmdtab *cmdtbl;
    struct ahci_fis_h2d *fis;
    int cmdslot, status;

    buf = vm_alloc_frame(1);
    if (buf == 0) {
        pr_trace("failed to alloc frame\n");
        return -ENOMEM;
    }

    cmdslot = ahci_alloc_cmdslot(hba, port);
    if (cmdslot < 0) {
        pr_trace("failed to alloc cmdslot\n");
        vm_free_frame(buf, 1);
        return cmdslot;
    }

    base = ahci_cmdbase(port);
    base += cmdslot * sizeof(*cmdhdr);

    /* Setup the command header */
    cmdhdr = PHYS_TO_VIRT(base);
    cmdhdr->w = 0;
    cmdhdr->cfl = sizeof(struct ahci_fis_h2d) / 4;
    cmdhdr->prdtl = 1;

    cmdtbl = PHYS_TO_VIRT(cmdhdr->ctba);
    cmdtbl->prdt[0].dba = buf;
    cmdtbl->prdt[0].dbc = 511;
    cmdtbl->prdt[0].i = 0;

    fis = (void *)&cmdtbl->cfis;
    fis->command = ATA_CMD_IDENTIFY;
    fis->c = 1;
    fis->type = FIS_TYPE_H2D;

    if ((status = ahci_submit_cmd(hba, port, cmdslot)) != 0) {
        goto done;
    }

    ahci_dump_identity(PHYS_TO_VIRT(buf));
done:
    vm_free_frame(buf, 1);
    return status;
}

/*
 * Send a read/write command to a SATA drive
 *
 * @hba: Host bus adapter of target port
 * @dev: Device to send over
 * @sio: System I/O descriptor
 * @write: If true, data pointed to by `sio` will be written
 *
 * XXX: - The `len` field in `sio` is block relative, in other words,
 *        set to 1 to read one block (512 bytes per block), etc.
 *
 *      - The `offset` field in `sio` is the LBA address.
 */
static int
ahci_sata_rw(struct ahci_hba *hba, struct hba_device *dev, struct sio_txn *sio,
    bool write)
{
    paddr_t base, buf;
    char *p, *dest;
    bool dcdr_hit = false;
    struct hba_port *port;
    struct dcdr_lookup dcd_lookup;
    struct dcd *dcd;
    struct ahci_cmd_hdr *cmdhdr;
    struct ahci_cmdtab *cmdtbl;
    struct ahci_fis_h2d *fis;
    int cmdslot, status;
    size_t nblocks, cur_lba;
    size_t len;

    if (sio == NULL) {
        return -EINVAL;
    }
    if (sio->len == 0 || sio->buf == NULL) {
        return -EINVAL;
    }

    port = dev->io;

    /*
     * Compute how many blocks can be cached.
     *
     * XXX: We do not want to fill the entire DCDR
     *      with a single drive read to reduce the
     *      frequency of DCDR evictions.
     *
     * TODO: We should also take advantage of logical
     *       block coalescing.
     */
    nblocks = sio->len;
    if (nblocks >= AHCI_DCDR_CAP) {
        nblocks = AHCI_DCDR_CAP / 2;
    }

    /*
     * If we are reading the drive, see if we have
     * anything in the cache.
     *
     * XXX: If there is a break in the cache and we
     *      have a miss inbetween, other DCDs are
     *      ignored. Wonder how we can mitigate
     *      fragmentation.
     */
    cur_lba = sio->offset;
    len = sio->len;
    for (size_t i = 0; i < nblocks && !write; ++i) {
        status = dcdr_lookup(dev->dcdr, cur_lba, &dcd_lookup);
        if (status != 0) {
            break;
        }
        if (len == 0) {
            break;
        }

        dcdr_hit = true;
        dcd = dcd_lookup.dcd_res;

        /* Hit, copy the cached data */
        dest = &((char *)sio->buf)[i * 512];
        p = dcd->block;
        memcpy(dest, p, 512);

        ++cur_lba;
        --len;
    }

    /* Did we get everything already? */
    if (len == 0) {
        return 0;
    }

    buf = VIRT_TO_PHYS(sio->buf);
    cmdslot = ahci_alloc_cmdslot(hba, port);
    if (cmdslot < 0) {
        pr_trace("failed to alloc cmdslot\n");
        return cmdslot;
    }

    base = ahci_cmdbase(port);
    base += cmdslot * sizeof(*cmdhdr);

    /* Setup the command header */
    cmdhdr = PHYS_TO_VIRT(base);
    cmdhdr->w = write;
    cmdhdr->cfl = sizeof(struct ahci_fis_h2d) / 4;
    cmdhdr->prdtl = 1;

    cmdtbl = PHYS_TO_VIRT(cmdhdr->ctba);
    cmdtbl->prdt[0].dba = buf;
    cmdtbl->prdt[0].dbc = (sio->len << 9) - 1;
    cmdtbl->prdt[0].i = 0;

    fis = (void *)&cmdtbl->cfis;
    fis->command = write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
    fis->c = 1;
    fis->type = FIS_TYPE_H2D;
    fis->device = (1 << 6); /* LBA */

    /* Setup LBA */
    fis->lba0 = cur_lba & 0xFF;
    fis->lba1 = (cur_lba >> 8) & 0xFF;
    fis->lba2 = (cur_lba >> 16) & 0xFF;
    fis->lba3 = (cur_lba >> 24) & 0xFF;
    fis->lba4 = (cur_lba >> 32) & 0xFF;
    fis->lba5 = (cur_lba >> 40) & 0xFF;

    /* Setup count */
    fis->countl = len & 0xFF;
    fis->counth = (len >> 8) & 0xFF;

    if ((status = ahci_submit_cmd(hba, port, cmdslot)) != 0) {
        return status;
    }

    /* Don't cache again on hit */
    if (!write && dcdr_hit) {
        return 0;
    }

    /* Cache our read */
    for (size_t i = 0; i < nblocks; ++i) {
        cur_lba = sio->offset + i;
        p = sio->buf;
        dcdr_cachein(dev->dcdr, &p[i * 512], cur_lba);
    }
    return 0;
}

static int
sata_dev_rw(dev_t dev, struct sio_txn *sio, bool write)
{
    const size_t BSIZE = 512;
    struct sio_txn wr_sio;
    struct hba_device *devp;
    size_t block_count, len;
    off_t block_off, read_off;
    char *buf;
    int status;

    if (sio == NULL) {
        return -EINVAL;
    }
    if (sio->len == 0 || sio->buf == NULL) {
        return -EINVAL;
    }
    if (dev >= devs_max) {
        return -ENODEV;
    }

    devp = &devs[dev];
    if (__unlikely(devp == NULL)) {
        return -ENODEV;
    }

    /* Compute block count and offset */
    block_count = ALIGN_UP(sio->len, BSIZE);
    block_count /= BSIZE;
    block_off = sio->offset / BSIZE;

    /* Allocate internal buffer */
    len = block_count * BSIZE;
    buf = dynalloc_memalign(len, 0x1000);
    if (buf == NULL) {
        return -ENOMEM;
    }

    /* Copy SIO buffer if write */
    if (write) {
        memset(buf, 0, len);
        memcpy(buf, sio->buf, sio->len);
    }

    /*
     * Perform the r/w operation and copy internal buffer
     * out if this is a read operation.
     */
    wr_sio.buf = buf;
    wr_sio.len = block_count;
    wr_sio.offset = block_off;
    status = ahci_sata_rw(&g_hba, devp, &wr_sio, write);
    if (status == 0 && !write) {
        read_off = sio->offset & (BSIZE - 1);
        memcpy(sio->buf, buf + read_off, sio->len);
    }

    dynfree(buf);
    return status;
}

/*
 * Device interface read
 */
static int
ahci_dev_read(dev_t dev, struct sio_txn *sio, int flags)
{
    return sata_dev_rw(dev, sio, false);
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
    char devname[128];
    struct hba_memspace *abar = hba->io;
    struct hba_port *port;
    struct hba_device *dp;
    struct ctlfs_dev dev;
    size_t clen, pagesz;
    uint32_t lo, hi, ssts;
    uint8_t det;
    paddr_t fra, cmdlist, tmp;
    int error;

    pagesz = DEFAULT_PAGESIZE;
    port = &abar->ports[portno];

    /* Is anything on the port? */
    ssts = mmio_read32(&port->ssts);
    det = AHCI_PXSCTL_DET(ssts);
    switch (det) {
    case AHCI_DET_NULL:
        /* No device attached */
        return 0;
    case AHCI_DET_PRESENT:
        if ((error = hba_port_reset(hba, port)) < 0) {
            pr_trace("failed to reset port %d\n", portno);
            return error;
        }
        break;
    }

    pr_trace("found device @ port %d\n", portno);
    dp = &devs[portno];
    dp->io = port;
    dp->hba = hba;
    dp->dev = portno;

    dp->dcdr = dcdr_alloc(512, AHCI_DCDR_CAP);
    if (dp->dcdr == NULL) {
        pr_error("failed to alloc dcdr\n");
        return -ENOMEM;
    }

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

    mmio_write32(&port->serr, 0xFFFFFFFF);

    if ((error = hba_port_start(port)) < 0) {
        for (int i = 0; i < hba->nslots; ++i) {
            vm_free_frame(dp->cmdlist[i].ctba, 1);
        }
        vm_free_frame(cmdlist, clen);
        vm_free_frame(fra, 1);
        pr_trace("failed to start port %d\n", portno);
        return error;
    }

    ahci_identify(hba, port);

    if (hba->major == 0) {
        hba->major = dev_alloc_major();
    }
    dp->dev = dev_alloc(hba->major);
    snprintf(devname, sizeof(devname), "sd%d", dp->dev);

    /* Register the device */
    dev_register(hba->major, dp->dev, &ahci_bdevsw);
    pr_trace("drive @ /dev/%s\n", devname);

    /* Register a control node */
    dev.mode = 0444;
    ctlfs_create_node(devname, &dev);
    pr_trace("drive control @ /ctl/%s/\n", devname);

    /* Register control files */
    dev.devname = devname;
    dev.ops = &g_sata_bsize_ops;
    ctlfs_create_entry("bsize", &dev);
    return devfs_create_entry(devname, hba->major, dp->dev, 0444);
}

/*
 * Scan the HBA for implemented ports
 */
static int
ahci_hba_scan(struct ahci_hba *hba)
{
    struct hba_memspace *abar = hba->io;
    uint32_t pi;
    size_t len;

    len = hba->nports * sizeof(struct hba_device);
    devs_max = hba->nports;
    if ((devs = dynalloc(len)) == NULL) {
        pr_trace("failed to allocate dev descriptors\n");
        return -ENOMEM;
    }

    memset(devs, 0, len);
    pi = mmio_read32(&abar->pi);
    for (int i = 0; i < sizeof(pi) * 8; ++i) {
        if (ISSET(pi, BIT(i))) {
            ahci_init_port(hba, i);
        }
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
    hba->sss = AHCI_CAP_SSS(cap);

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

/*
 * Init PCI related controller bits
 */
static void
ahci_init_pci(void)
{
    uint32_t tmp;

    /* Enable bus mastering and MMIO */
    tmp = pci_readl(ahci_dev, PCIREG_CMDSTATUS);
    tmp |= (PCI_BUS_MASTERING | PCI_MEM_SPACE);
    pci_writel(ahci_dev, PCIREG_CMDSTATUS, tmp);
}

static int
ahci_init(void)
{
    struct pci_lookup lookup;
    int status;
    void *abar_vap = NULL;

    g_hba.major = 0;
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

    ahci_init_pci();
    g_hba.io = (struct hba_memspace*)abar_vap;
    ahci_hba_init(&g_hba);
    return 0;
}

static struct bdevsw ahci_bdevsw = {
    .read = ahci_dev_read,
    .write = nowrite
};

DRIVER_EXPORT(ahci_init);

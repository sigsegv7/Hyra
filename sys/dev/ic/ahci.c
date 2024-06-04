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

#include <sys/driver.h>
#include <sys/timer.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <sys/mmio.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <fs/devfs.h>
#include <dev/pci/pci.h>
#include <dev/ic/ahciregs.h>
#include <dev/ic/ahcivar.h>
#include <machine/bus.h>
#include <vm/vm.h>
#include <string.h>

__KERNEL_META("$Hyra$: ahci.c, Ian Marco Moffett, "
              "AHCI driver");

#define pr_trace(fmt, ...) kprintf("ahci: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static TAILQ_HEAD(, ahci_device) sata_devs;

static struct pci_device *dev;
static struct timer driver_tmr;
static struct mutex io_lock;
static size_t dev_count = 0;

static bool
is_word_aligned(void *ptr)
{
    return (((uintptr_t)ptr) & 1) == 0;
}

/*
 * Fetch a SATA device with a SATA device
 * minor.
 *
 * @dev_minor: SATA device minor.
 */
static struct ahci_device *
ahci_get_sata(dev_t dev_minor)
{
    struct ahci_device *dev;

    TAILQ_FOREACH(dev, &sata_devs, link) {
        if (dev->minor == dev_minor) {
            return dev;
        }
    }

    return NULL;
}

/*
 * Poll register to have `bits' set/unset.
 *
 * @reg: Register to poll.
 * @bits: Bits expected to be set/unset.
 * @pollset: True to poll as set.
 */
static int
ahci_poll_reg32(volatile uint32_t *reg, uint32_t bits, bool pollset)
{
    uint32_t time_waited = 0;
    uint32_t val;
    bool tmp;

    for (;;) {
        val = mmio_read32(reg);
        tmp = (pollset) ? __TEST(val, bits) : !__TEST(val, bits);
        if (tmp)
            break;
        if (time_waited >= AHCI_TIMEOUT)
            /* Timeout */
            return -1;

        driver_tmr.msleep(10);
        time_waited += 10;
    }

    return 0;
}

/*
 * Put the HBA in AHCI mode.
 */
static inline void
ahci_set_ahci(struct ahci_hba *hba)
{
    struct hba_memspace *abar = hba->abar;
    uint32_t ghc;

    /* Enable AHCI mode */
    ghc = mmio_read32(&abar->ghc);
    ghc |= AHCI_GHC_AE;
    mmio_write32(&abar->ghc, ghc);
}

/*
 * Reset the HBA with GHC.HR
 *
 * XXX: The spec states that all port registers *except*
 *      PxFB, PxFBU, PxCLB, PxCLBU are reset.
 */
static int
ahci_hba_reset(struct ahci_hba *hba)
{
    struct hba_memspace *abar = hba->abar;
    uint32_t ghc;
    uint8_t attempts = 0;
    int status;

    ghc = mmio_read32(&abar->ghc);
    ghc |= AHCI_GHC_HR;
    mmio_write32(&abar->ghc, ghc);

    /*
     * Poll the GHC.HR bit. The HBA is supposed to flip
     * it back to zero once the reset is complete. If
     * the HBA does not do this, something is screwed
     * up.
     *
     * XXX: We do this twice in case of slow hardware...
     */
    while ((attempts++) < 2) {
        status = ahci_poll_reg32(&abar->ghc, AHCI_GHC_HR, false);
        if (status == 0) {
            break;
        }
    }

    /* We hope this doesn't happen */
    if (status != 0) {
        pr_error("HBA reset failure: GHC.HR stuck (HBA hung)\n");
        return status;
    }

    ahci_set_ahci(hba);
    return 0;
}

/*
 * Stop port and put it in an idle state.
 */
static int
ahci_stop_port(struct hba_port *port)
{
    const uint32_t RUN_MASK = (AHCI_PXCMD_FR | AHCI_PXCMD_CR);
    uint32_t cmd = mmio_read32(&port->cmd);

    /* Check if it is already stopped */
    if (!__TEST(cmd, RUN_MASK))
        return 0;

    /*
     * Stop the FIS receive and disable proessing
     * of the command list.
     */
    cmd &= ~(AHCI_PXCMD_ST | AHCI_PXCMD_FRE);
    mmio_write32(&port->cmd, cmd);
    return ahci_poll_reg32(&port->cmd, RUN_MASK, false);
}

/*
 * Put a port in a running state.
 */
static int
ahci_start_port(struct hba_port *port)
{
    const uint32_t RUN_MASK = (AHCI_PXCMD_FR | AHCI_PXCMD_CR);
    uint32_t cmd = mmio_read32(&port->cmd);

    /* Check if it is already running */
    if (__TEST(cmd, RUN_MASK))
        return 0;

    /* Start everything up */
    cmd |= (AHCI_PXCMD_ST | AHCI_PXCMD_FRE);
    mmio_write32(&port->cmd, cmd);
    return ahci_poll_reg32(&port->cmd, RUN_MASK, true);
}

/*
 * Check if a port is active.
 *
 * @port: Port to check.
 */
static bool
ahci_port_active(struct hba_port *port)
{
    uint32_t ssts;
    uint8_t det, ipm;

    ssts = mmio_read32(&port->ssts);
    det = AHCI_PXSSTS_DET(ssts);
    ipm = AHCI_PXSSTS_IPM(ssts);
    return (det == AHCI_DET_COMM && ipm == AHCI_IPM_ACTIVE);
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

    pr_trace("DRIVE MODEL NUMBER: %s\n", model_number);
    pr_trace("DRIVE SERIAL NUMBER: %s\n", serial_number);
}

/*
 * Allocate a command slot.
 */
static int
ahci_alloc_cmdslot(struct ahci_hba *hba, struct hba_port *port)
{
    uint32_t slotlist = (port->ci | port->sact);

    for (uint16_t i = 0; i < hba->ncmdslots; ++i) {
        if (!__TEST(slotlist, i))
            return i;
    }

    return -1;
}

/*
 * Submit a command to a device
 *
 * @port: Port of device to submit command to
 * @cmdslot: Command slot.
 */
static int
ahci_submit_cmd(struct ahci_hba *hba, struct hba_port *port, uint8_t cmdslot)
{
    const uint32_t BUSY_BITS = (AHCI_PXTFD_BSY | AHCI_PXTFD_DRQ);
    const uint8_t MAX_ATTEMPTS = 3;
    uint8_t attempts = 0;
    int status = 0;

    /*
     * Ensure the port isn't busy before we try to send
     * any commands. Spin on BSY and DRQ bits until they
     * become unset or we timeout.
     */
    if (ahci_poll_reg32(&port->tfd, BUSY_BITS, false) < 0) {
        pr_error("Command failed: Port is busy! (slot=%d)\n", cmdslot);
        return -EBUSY;
    }

    /* Activate the command slot */
    mutex_acquire(&io_lock);
    mmio_write32(&port->ci, __BIT(cmdslot));

    /*
     * Wait for completion. since this might take a bit, we
     * give it a few attempts in case it doesn't finish
     * right away.
     */
    while ((attempts++) < MAX_ATTEMPTS) {
        status = ahci_poll_reg32(&port->ci, __BIT(cmdslot), false);
        if (status == 0) {
            break;
        }
    }

    /* Did we timeout? */
    if (status != 0) {
        pr_error("IDENTIFY timeout: slot %d still set!\n", cmdslot);
    }

    mutex_release(&io_lock);
    return status;
}

static int
ahci_sata_rw(struct ahci_hba *hba, struct hba_port *port, struct sio_txn *sio,
             bool write)
{
    paddr_t buf_phys;
    struct ahci_cmd_hdr *cmdhdr;
    struct ahci_cmdtab *cmdtbl;
    struct ahci_fis_h2d *fis;
    int cmdslot, status;

    if (sio->buf == NULL || !is_word_aligned(sio->buf))
        return -EINVAL;
    if (sio->len == 0)
        return -EINVAL;

    buf_phys = VIRT_TO_PHYS(sio->buf);
    cmdslot = ahci_alloc_cmdslot(hba, port);

    /* Setup command header */
    cmdhdr = PHYS_TO_VIRT(port->clb + cmdslot * sizeof(struct ahci_cmd_hdr));
    cmdhdr->w = 0;
    cmdhdr->cfl = sizeof(struct ahci_fis_h2d) / 4;
    cmdhdr->prdtl = 1;

    /* Setup physical region descriptor */
    cmdtbl = PHYS_TO_VIRT(cmdhdr->ctba);
    cmdtbl->prdt[0].dba = buf_phys;
    cmdtbl->prdt[0].dbc = (sio->len << 9) - 1;
    cmdtbl->prdt[0].i = 0;

    /* Setup command FIS */
    fis = (void *)&cmdtbl->cfis;
    fis->type = FIS_TYPE_H2D;
    fis->command = write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
    fis->c = 1;
    fis->device = (1 << 6);

    /* Setup LBA */
    fis->lba0 = sio->offset & 0xFF;
    fis->lba1 = (sio->offset >> 8) & 0xFF;
    fis->lba2 = (sio->offset >> 16) & 0xFF;
    fis->lba3 = (sio->offset >> 24) & 0xFF;
    fis->lba4 = (sio->offset >> 32) & 0xFF;
    fis->lba5 = (sio->offset >> 40) & 0xFF;

    /* Setup count */
    fis->countl = sio->len & 0xFF;
    fis->counth = (sio->len >> 8) & 0xFF;

    if ((status = ahci_submit_cmd(hba, port, cmdslot)) != 0) {
        return status;
    }

    return 0;
}

/*
 * Send the IDENTIFY command to a device and
 * log info for debugging purposes.
 */
static int
ahci_identify(struct ahci_hba *hba, struct hba_port *port)
{
    paddr_t buf_phys;
    struct ahci_cmd_hdr *cmdhdr;
    struct ahci_cmdtab *cmdtbl;
    struct ahci_fis_h2d *fis;
    int cmdslot;
    void *buf;
    int status = 0;

    cmdslot = ahci_alloc_cmdslot(hba, port);
    buf_phys = vm_alloc_pageframe(1);
    buf = PHYS_TO_VIRT(buf_phys);

    if (buf_phys == 0) {
        status = -ENOMEM;
        goto done;
    }

    if (cmdslot < 0) {
        status = cmdslot;
        goto done;
    }

    memset(buf, 0, vm_get_page_size());
    cmdhdr = PHYS_TO_VIRT(port->clb + cmdslot * sizeof(struct ahci_cmd_hdr));
    cmdhdr->w = 0;
    cmdhdr->cfl = sizeof(struct ahci_fis_h2d) / 4;
    cmdhdr->prdtl = 1;

    cmdtbl = PHYS_TO_VIRT(cmdhdr->ctba);
    cmdtbl->prdt[0].dba = VIRT_TO_PHYS(buf);
    cmdtbl->prdt[0].dbc = 511;
    cmdtbl->prdt[0].i = 0;

    fis = (void *)&cmdtbl->cfis;
    fis->command = ATA_CMD_IDENTIFY;
    fis->c = 1;
    fis->type = FIS_TYPE_H2D;

    if ((status = ahci_submit_cmd(hba, port, cmdslot)) != 0) {
        goto done;
    }

    ahci_dump_identity(buf);
done:
    vm_free_pageframe(VIRT_TO_PHYS(buf), 1);
    return status;
}

/*
 * Device interface read/write helper
 */
static int
sata_dev_rw(struct device *dev, struct sio_txn *sio, bool write)
{
    struct ahci_device *sata;

    if (sio == NULL)
        return -1;
    if (sio->buf == NULL)
        return -1;

    sata = ahci_get_sata(dev->minor);

    if (sata == NULL)
       return -1;

    return ahci_sata_rw(sata->hba, sata->port, sio, write);
}

/*
 * Device interface read
 */
static int
sata_dev_read(struct device *dev, struct sio_txn *sio)
{
    return sata_dev_rw(dev, sio, false);
}

/*
 * Device interface write
 */
static int
sata_dev_write(struct device *dev, struct sio_txn *sio)
{
    return sata_dev_rw(dev, sio, true);
}

/*
 * Device interface open
 */
static int
sata_dev_open(struct device *dev)
{
    return 0;
}

/*
 * Device interface close
 */
static int
sata_dev_close(struct device *dev)
{
    return 0;
}

/*
 * Register a SATA device to the rest of the system
 * and expose to userland as a device file.
 */
static int
ahci_sata_register(struct ahci_hba *hba, struct hba_port *port)
{
    char devname[128];
    struct device *dev = NULL;
    struct ahci_device *sata = NULL;
    dev_t dev_id;
    dev_t major;

    sata = dynalloc(sizeof(struct ahci_device));
    if (sata == NULL) {
        return -ENOMEM;
    }

    dev_id = ++dev_count;
    major = device_alloc_major();

    dev = device_alloc();
    dev->open = sata_dev_open;
    dev->close = sata_dev_close;
    dev->read = sata_dev_read;
    dev->write = sata_dev_write;
    dev->blocksize = 512;
    device_create(dev, dev_id, major);

    sata->port = port;
    sata->hba = hba;
    sata->minor = dev->minor;

    snprintf(devname, sizeof(devname), "sd%d", dev_id);
    devfs_add_dev(devname, dev);
    TAILQ_INSERT_TAIL(&sata_devs, sata, link);
    return 0;
}

/*
 * Init a single port.
 *
 * @port: Port to init.
 */
static int
ahci_init_port(struct ahci_hba *hba, struct hba_port *port, size_t portno)
{
    paddr_t tmp;
    struct ahci_cmd_hdr *cmdlist;
    void *fis;
    size_t cmdlist_size, pagesize;
    uint32_t sig;
    uint8_t ncmdslots;
    int status = 0;

    sig = mmio_read32(&port->sig);
    status = ahci_stop_port(port);
    if (status != 0) {
        pr_trace("Failed to stop port %d\n", portno);
        return status;
    }

    /* Try to report device type based on signature */
    switch (sig) {
    case AHCI_SIG_PM:
        pr_trace("Port %d has port multiplier signature\n", portno);
        return 0;   /* TODO */
    case AHCI_SIG_ATA:
        pr_trace("Port %d has ATA signature (SATA drive)\n", portno);
        break;
    default:
        return 0;   /* TODO */
    }

    ncmdslots = hba->ncmdslots;
    pagesize = vm_get_page_size();

    /* Allocate our command list */
    cmdlist_size = __ALIGN_UP(ncmdslots * AHCI_CMDENTRY_SIZE, pagesize);
    tmp = vm_alloc_pageframe(cmdlist_size / pagesize);
    cmdlist = PHYS_TO_VIRT(tmp);
    if (tmp == 0) {
        pr_trace("Failed to allocate cmdlist\n");
        status = -ENOMEM;
        goto done;
    }

    tmp = vm_alloc_pageframe(1);
    fis = PHYS_TO_VIRT(tmp);
    if (tmp == 0) {
        pr_trace("Failed to allocate FIS\n");
        status = -ENOMEM;
        goto done;
    }

    memset(cmdlist, 0, cmdlist_size);
    memset(fis, 0, AHCI_FIS_SIZE);
    hba->cmdlist = cmdlist;

    /* Set the registers */
    port->clb = VIRT_TO_PHYS(cmdlist);
    port->fb = VIRT_TO_PHYS(fis);

    for (int i = 0; i < ncmdslots; ++i) {
        cmdlist[i].prdtl = 1;
        cmdlist[i].ctba = vm_alloc_pageframe(1);
    }

    /* Now try to start up the port */
    if ((status = ahci_start_port(port)) != 0) {
        pr_trace("Failed to start port %d\n", portno);
        goto done;
    }

    ahci_identify(hba, port);
    ahci_sata_register(hba, port);
done:
    if (status != 0 && cmdlist != NULL)
        vm_free_pageframe(port->clb, cmdlist_size / pagesize);
    if (status != 0 && fis != NULL)
        vm_free_pageframe(port->fb, 1);

    return status;
}

/*
 * Hard reset port and reinitialize
 * link.
 */
static int
ahci_reset_port(struct hba_port *port)
{
    uint32_t sctl, ssts;

    /*
     * Some odd behaviour may occur if a COMRESET is sent
     * to the port while it is in an idle state...
     * A workaround to this is to bring the port up
     * then immediately transmit the COMRESET to the device.
     */
    ahci_start_port(port);
    sctl = mmio_read32(&port->sctl);

    /* Transmit COMRESET for ~2ms */
    sctl = (sctl & ~0xF) | AHCI_DET_COMRESET;
    mmio_write32(&port->sctl, sctl);
    driver_tmr.msleep(2);

    /* Stop transmission of COMRESET */
    sctl &= ~AHCI_DET_COMRESET;
    mmio_write32(&port->sctl, sctl);

    /*
     * Give around ~150ms for the link to become
     * reestablished. Then make sure that it is
     * actually established by checking PxSSTS.DET
     */
    driver_tmr.msleep(150);
    ssts = mmio_read32(&port->ssts);
    if (AHCI_PXSSTS_DET(ssts) != AHCI_DET_COMM) {
        return -1;
    }

    return 0;
}

/*
 * Sets up devices connected to the physical ports
 * on the HBA.
 *
 * XXX: Since this is called after ahci_init_hba() which also
 *      resets the HBA, we'll need to reestablish the link
 *      between the devices and the HBA.
 */
static int
ahci_init_ports(struct ahci_hba *hba)
{
    struct hba_memspace *abar = hba->abar;
    uint32_t ports_impl;
    struct hba_port *port;

    pr_trace("HBA supports max %d port(s)\n", hba->nports);
    ports_impl = mmio_read32(&abar->pi);

    /* Initialize active ports */
    for (size_t i = 0; i < sizeof(abar->pi) * 8; ++i) {
        if (!__TEST(ports_impl, __BIT(i))) {
            continue;
        }

        port = &abar->ports[i];
        if (ahci_reset_port(port) != 0) {
            continue;
        }

        if (ahci_port_active(port)) {
            ahci_init_port(hba, port, i);
        }
    }

    return 0;
}

/*
 * Sets up the HBA
 */
static int
ahci_init_hba(struct ahci_hba *hba)
{
    struct hba_memspace *abar = hba->abar;
    uint32_t cap;

    /* Reset the HBA to ensure it is a known state */
    ahci_hba_reset(hba);

    /* Setup HBA structure and save some state */
    cap = mmio_read32(&abar->cap);
    hba->ncmdslots = AHCI_CAP_NCS(cap) + 1;
    hba->nports = AHCI_CAP_NP(cap) + 1;

    ahci_init_ports(hba);
    return 0;
}

static int
ahci_init(void)
{
    int status;
    uint16_t cmdreg_bits;
    uint32_t bar_size;
    struct ahci_hba hba = {0};
    struct pci_lookup ahci_lookup = {
        .pci_class = 0x01,
        .pci_subclass = 0x06
    };

    dev = pci_get_device(ahci_lookup, PCI_CLASS | PCI_SUBCLASS);

    if (dev == NULL) {
        return -1;
    }

    cmdreg_bits = PCI_BUS_MASTERING | PCI_MEM_SPACE;
    pci_set_cmdreg(dev, cmdreg_bits);

    if (req_timer(TIMER_GP, &driver_tmr) != TMRR_SUCCESS) {
        pr_error("Failed to fetch general purpose timer\n");
        return -1;
    }

    if (driver_tmr.msleep == NULL) {
        pr_error("Timer does not have msleep()\n");
        return -1;
    }

    if ((bar_size = pci_bar_size(dev, 5)) == 0) {
        pr_error("Failed to fetch BAR size\n");
        return -1;
    }

    status = bus_map(dev->bar[5], bar_size, 0, (void *)&hba.abar);
    if (status != 0) {
        pr_error("Failed to map BAR into higher half\n");
        return -1;
    }

    pr_trace("AHCI HBA memspace @ 0x%p\n", hba.abar);
    TAILQ_INIT(&sata_devs);
    ahci_init_hba(&hba);
    return 0;
}

DRIVER_EXPORT(ahci_init);

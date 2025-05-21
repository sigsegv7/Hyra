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
#include <sys/param.h>
#include <sys/driver.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/mmio.h>
#include <dev/timer.h>
#include <dev/usb/xhciregs.h>
#include <dev/usb/xhcivar.h>
#include <dev/pci/pci.h>
#include <dev/acpi/acpi.h>
#include <vm/physmem.h>
#include <vm/dynalloc.h>
#include <assert.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("xhci: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

/* Debug macro */
#if defined(XHCI_DEBUG)
#define pr_debug(...) pr_trace(__VA_ARGS__)
#else
#define pr_debug(...) __nothing
#endif  /* XHCI_DEBUG */

static struct pci_device *hci_dev;
static struct timer tmr;

static int
xhci_intr(void *sf)
{
    pr_trace("received xHCI interrupt (via PCI MSI-X)\n");
    return 1;   /* handled */
}

/*
 * Get port status and control register for a specific
 * port.
 */
static inline uint32_t *
xhci_get_portsc(struct xhci_hc *hc, uint8_t portno)
{
    if (portno > hc->maxports) {
        portno = hc->maxports;
    }

    return PTR_OFFSET(hc->opregs, 0x400 + (0x10 * (portno - 1)));
}

static int
xhci_poll32(volatile uint32_t *reg, uint32_t bits, bool pollset)
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
        if (elapsed_msec > XHCI_TIMEOUT) {
            return -ETIME;
        }
    }

    return val;
}

/*
 * Parse xHCI extended caps.
 */
static int
xhci_parse_ecp(struct xhci_hc *hc)
{
    struct xhci_caps *caps = XHCI_CAPS(hc->base);
    struct xhci_proto *proto;
    uint32_t *p, val, dword2;
    uint32_t cap_ptr = XHCI_ECP(caps->hccparams1);

    p = PTR_OFFSET(hc->base, cap_ptr * 4);
    while (cap_ptr != 0) {
        val = mmio_read32(p);
        dword2 = *((uint32_t *)PTR_OFFSET(p, 8));

        /* Get the next cap */
        p = PTR_OFFSET(p, XHCI_PROTO_NEXT(val) * 4);
        cap_ptr = XHCI_PROTO_NEXT(val);

        switch (XHCI_PROTO_ID(val)) {
        case XHCI_ECAP_PROTO:
            if (hc->protocnt > XHCI_MAX_PROTOS) {
                /* Too many protocols */
                return 0;
            }

            proto = &hc->protos[hc->protocnt++];
            proto->major = XHCI_PROTO_MAJOR(val);
            proto->port_count = XHCI_PROTO_PORTCNT(dword2);
            proto->port_start = XHCI_PROTO_PORTOFF(dword2);

            pr_trace("USB %d port start: %d\n",
                proto->major, proto->port_start);
            pr_trace("USB %d port count: %d\n",
                proto->major, proto->port_count);
            break;
        case XHCI_ECAP_USBLEGSUP:
            /* Begin xHC BIOS handoff to us */
            if (!ISSET(hc->quirks, XHCI_QUIRK_HANDOFF)) {
                pr_trace("establishing xHC ownership...\n");
                val |= XHCI_OS_SEM;
                mmio_write32(p, val);

                /* Ensure the xHC responded correctly */
                if (xhci_poll32(p, XHCI_OS_SEM, 1) < 0)
                    return -EIO;
                if (xhci_poll32(p, XHCI_BIOS_SEM, 0) < 0)
                    return -EIO;
            }

            break;
        }
    }
    return 0;
}

/*
 * Init set of scratchpad buffers for the
 * xHC.
 */
static int
xhci_init_scratchpads(struct xhci_hc *hc)
{
    struct xhci_caps *caps = XHCI_CAPS(hc->base);
    uint16_t max_bufs_lo, max_bufs_hi;
    uint16_t max_bufs;
    uintptr_t *bufarr, tmp;

    max_bufs_lo = XHCI_MAX_SP_LO(caps->hcsparams1);
    max_bufs_hi = XHCI_MAX_SP_HI(caps->hcsparams2);
    max_bufs = (max_bufs_hi << 5) | max_bufs_lo;
    if (max_bufs == 0) {
        /*
         * Some emulators like QEMU don't require any
         * scratchpad buffers.
         */
        return 0;
    }

    pr_trace("using %d pages for xHC scratchpads\n");
    bufarr = dynalloc_memalign(sizeof(uintptr_t)*max_bufs, 0x1000);
    if (bufarr == NULL) {
        pr_error("failed to allocate scratchpad buffer array\n");
        return -1;
    }

    for (size_t i = 0; i < max_bufs; ++i) {
        tmp = vm_alloc_frame(1);
        memset(PHYS_TO_VIRT(tmp), 0, 0x1000);
        if (tmp == 0) {
            /* TODO: Shutdown, free memory */
            pr_error("failed to fill scratchpad buffer array\n");
            return -1;
        }
        bufarr[i] = tmp;
    }

    hc->dcbaap[0] = VIRT_TO_PHYS(bufarr);
    return 0;
}

/*
 * Allocate device context base array.
 */
static uintptr_t
xhci_alloc_dcbaa(struct xhci_hc *hc)
{
    size_t size;

    size = sizeof(uintptr_t) * hc->maxslots;
    hc->dcbaap = dynalloc_memalign(size, 0x1000);
    __assert(hc->dcbaap != NULL);
    return VIRT_TO_PHYS(hc->dcbaap);
}

/*
 * Initialize MSI-X interrupts.
 */
static int
xhci_init_msix(struct xhci_hc *hc)
{
    struct msi_intr intr;

    intr.name = "xHCI MSI-X";
    intr.handler = xhci_intr;
    return pci_enable_msix(hci_dev, &intr);
}

/*
 * Sets up the event ring.
 */
static void
xhci_init_evring(struct xhci_hc *hc)
{
    struct xhci_caps *caps = XHCI_CAPS(hc->base);
    struct xhci_evring_segment *segtab;
    uint64_t *erdp, *erstba, tmp;
    uint32_t *erst_size, *iman, *imod;
    void *runtime = XHCI_RTS(hc->base, caps->rtsoff);
    void *tmp_p;
    size_t size;

    segtab = PHYS_TO_VIRT(vm_alloc_frame(1));
    memset(segtab, 0, DEFAULT_PAGESIZE);

    /* Set the size of the event ring segment table */
    erst_size = PTR_OFFSET(runtime, XHCI_RT_ERSTSZ);
    mmio_write32(erst_size, 1);

    /* Allocate the event ring segment */
    size = XHCI_EVRING_LEN * XHCI_TRB_SIZE;
    tmp_p = PHYS_TO_VIRT(vm_alloc_frame(4));
    memset(tmp_p, 0, size);

    /* setup the event ring segment */
    segtab->base = VIRT_TO_PHYS(tmp_p);
    segtab->base = ((uintptr_t)segtab->base) + (2 * 4096) & ~0xF;
    segtab->size = XHCI_EVRING_LEN;

    /* Setup the event ring dequeue pointer */
    erdp = PTR_OFFSET(runtime, XHCI_RT_ERDP);
    mmio_write64(erdp, segtab->base);

    /* Point ERSTBA to our event ring segment */
    erstba = PTR_OFFSET(runtime, XHCI_RT_ERSTBA);
    mmio_write64(erstba, VIRT_TO_PHYS(segtab));
    hc->evring = PHYS_TO_VIRT(segtab->base);

    /* Setup interrupt moderation */
    imod = PTR_OFFSET(runtime, XHCI_RT_IMOD);
    mmio_write32(imod, XHCI_IMOD_DEFAULT);

    /* Enable interrupts */
    iman = PTR_OFFSET(runtime, XHCI_RT_IMAN);
    tmp = mmio_read32(iman);
    mmio_write32(iman, tmp | XHCI_IMAN_IE);
}

/*
 * Allocate command ring and sets 'hc->cmdring'
 * to the virtual address.
 *
 * Returns the physical address.
 */
static uintptr_t
xhci_alloc_cmdring(struct xhci_hc *hc)
{
    size_t size;

    size = XHCI_TRB_SIZE * XHCI_CMDRING_LEN;
    hc->cmdring = dynalloc_memalign(size, 0x1000);

    memset(hc->cmdring, 0, size);
    __assert(hc->cmdring != NULL);
    return VIRT_TO_PHYS(hc->cmdring);
}

/*
 * Perform an xHC reset and return 0 on
 * success.
 */
static int
xhci_reset(struct xhci_hc *hc)
{
    struct xhci_opregs *opregs = hc->opregs;
    uint32_t usbcmd;
    int error;

    /* Make sure a reset isn't already in progress */
    usbcmd = mmio_read32(&opregs->usbcmd);
    if (ISSET(usbcmd, USBCMD_HCRST))
        return -EBUSY;

    usbcmd |= USBCMD_HCRST;
    mmio_write32(&opregs->usbcmd, usbcmd);

    /* Wait until xHC finishes */
    error = xhci_poll32(&opregs->usbcmd, USBCMD_HCRST, false);
    if (error < 0) {
        pr_error("xhci_reset: xHC reset timeout\n");
        return error;
    }

    return 0;
}

/*
 * Enable or disable xHC interrupts.
 */
static void
xhci_set_intr(struct xhci_hc *hc, int enable)
{
    struct xhci_opregs *opregs = hc->opregs;
    uint32_t usbcmd;

    enable &= 1;
    usbcmd = mmio_read32(&opregs->usbcmd);

    if (enable == 1) {
        usbcmd |= USBCMD_INTE;
    } else {
        usbcmd &= ~USBCMD_INTE;
    }

    mmio_write32(&opregs->usbcmd, usbcmd);
}

/*
 * Start up the host controller and put it into
 * a running state. Returns 0 on success.
 */
static int
xhci_start_hc(struct xhci_hc *hc)
{
    struct xhci_opregs *opregs = hc->opregs;
    uint32_t usbcmd;

    /* Don't start up if we are already running */
    usbcmd = mmio_read32(&opregs->usbcmd);
    if (ISSET(usbcmd, USBCMD_RUN))
        return -EBUSY;

    usbcmd |= USBCMD_RUN;
    mmio_write32(&opregs->usbcmd, usbcmd);
    return 0;
}

static int
xhci_init_ports(struct xhci_hc *hc)
{
    const char *devtype;
    uint32_t *portsc_p;
    uint32_t portsc;

    for (size_t i = 1; i < hc->maxports; ++i) {
        portsc_p = xhci_get_portsc(hc, i);
        portsc = mmio_read32(portsc_p);

        /*
         * If the current connect status of a port is
         * set, we know we have some sort of device
         * connected to it.
         */
        if (ISSET(portsc, XHCI_PORTSC_CCS)) {
            devtype = (ISSET(portsc, XHCI_PORTSC_DR))
                ? "removable" : "non-removable";

            pr_trace("detected %s USB device on port %d\n", devtype, i);
            pr_trace("resetting port %d...\n", i);
            portsc |= XHCI_PORTSC_PR;
            mmio_write32(portsc_p, portsc);
        }
    }

    return 0;
}

/*
 * Initialize the xHCI controller.
 */
static int
xhci_init_hc(struct xhci_hc *hc)
{
    int error;
    uint8_t caplength;
    uint32_t config;
    uintptr_t dcbaap, cmdring;
    struct xhci_caps *caps;
    struct xhci_opregs *opregs;
    const char *vendor;

    /*
     * The firmware on some Dell machines handle the
     * xHCI BIOS/OS handoff very poorly. Updating the
     * the OS semaphore in the USBLEGSUP register will
     * result in the chipset firing off an SMI which is
     * supposed to perform the actual handoff.
     *
     * However, Dell is stupid as always and the machine
     * can get stuck in SMM which results in the machine
     * locking up in a *very* bad way. In other words, the
     * OS execution is literally halted and further SMIs like
     * thermal, power, and fan events are deferred forever
     * (no bueno!!). The best thing to do is to not perform
     * a handoff if the host board is by Dell (bad Dell!!).
     */
    vendor = acpi_oemid();
    if (memcmp(vendor, "DELL", 4) == 0) {
        pr_trace("detected xhc handoff quirk\n");
        hc->quirks |= XHCI_QUIRK_HANDOFF;
    }

    caps = (struct xhci_caps *)hc->base;
    caplength = mmio_read8(&caps->caplength);
    opregs = PTR_OFFSET(caps, caplength);

    hc->caps = caps;
    hc->opregs = opregs;

    /*
     * If the Operational Base is not dword aligned then
     * we can assume that perhaps the controller is faulty
     * and giving bogus values.
     */
    if (!PTR_ALIGNED(hc->opregs, 4)) {
        pr_error("xhci_init_hc: fatal: Got bad operational base\n");
        return -1;
    }

    pr_trace("resetting xHC chip...\n");
    if ((error = xhci_reset(hc)) != 0) {
        return error;
    }

    hc->maxslots = XHCI_MAXSLOTS(caps->hcsparams1);
    hc->maxports = XHCI_MAXPORTS(caps->hcsparams1);

    /* Set CONFIG.MaxSlotsEn to all of them */
    config = mmio_read32(&opregs->config);
    config |= hc->maxslots;
    mmio_write32(&opregs->config, config);

    /* Set device context base array */
    dcbaap = xhci_alloc_dcbaa(hc);
    mmio_write64(&opregs->dcbaa_ptr, dcbaap);

    /* Try to set up scratchpad buffer array */
    if ((error = xhci_init_scratchpads(hc)) != 0) {
        return error;
    }

    /* Setup the command ring */
    cmdring = xhci_alloc_cmdring(hc);
    mmio_write64(&opregs->cmd_ring, cmdring);
    hc->cr_cycle = 1;

    xhci_init_msix(hc);
    xhci_init_evring(hc);
    xhci_parse_ecp(hc);
    xhci_start_hc(hc);

    /* Allow the xHC to generate interrupts */
    xhci_set_intr(hc, 1);
    xhci_init_ports(hc);
    return 0;
}

static int
xhci_init(void)
{
    int error;
    struct xhci_hc xhc = {0};
    struct pci_lookup lookup = {
        .pci_class = 0x0C,
        .pci_subclass = 0x03
    };

    /* Find the host controller on the bus */
    hci_dev = pci_get_device(lookup, PCI_CLASS | PCI_SUBCLASS);
    if (hci_dev == NULL) {
        return -1;
    }

    if ((error = pci_map_bar(hci_dev, 0, (void *)&xhc.base)) != 0) {
        return error;
    }

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

    return xhci_init_hc(&xhc);
}

DRIVER_EXPORT(xhci_init);

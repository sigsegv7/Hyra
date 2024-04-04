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
#include <dev/pci/pci.h>
#include <sys/syslog.h>
#include <sys/timer.h>
#include <dev/usb/xhciregs.h>
#include <dev/usb/xhcivar.h>
#include <vm/physseg.h>
#include <vm/dynalloc.h>
#include <vm/page.h>
#include <vm/vm.h>
#include <string.h>
#include <assert.h>

__MODULE_NAME("xhci");
__KERNEL_META("$Hyra$: xhci.c, Ian Marco Moffett, "
              "xHCI driver");

static struct pci_device *hci_dev;
static struct timer driver_tmr;

static inline uint32_t *
xhci_get_portsc(struct xhci_hc *hc, uint8_t portno)
{
    if (portno > hc->maxports) {
        portno = hc->maxports;
    }
    return XHCI_BASE_OFF(hc->opregs, 0x400 + (0x10 * (portno - 1)));
}

/*
 * Set event ring segment table base
 * address.
 *
 * @hc: Host controler descriptor.
 * @pa: Physical address.
 */
static inline void
xhci_set_erst_base(struct xhci_hc *hc, uintptr_t pa)
{
    struct xhci_caps *caps = XHCI_CAPS(hc->base);
    void *runtime_base = XHCI_RTS(hc->base, caps->rtsoff);
    uintptr_t *erstba;

    /*
     * The spec states that the Event Ring Segment Table
     * Base Address register is at 'runtime_base + 0x30 +
     * (32 * interrupter)'. See the xHCI spec, section 5.5.2.3.2
     */
    erstba = XHCI_BASE_OFF(runtime_base, 0x30);
    *erstba = pa;
}

/*
 * Set up an xHCI event ring segment.
 */
static int
xhci_init_evring_segment(struct xhci_evring_segment *seg)
{
    size_t evring_size;
    void *event_ring;

    evring_size = XHCI_TRB_SIZE * XHCI_EVRING_LEN;
    event_ring = dynalloc_memalign(evring_size, 64);

    seg->base = VIRT_TO_PHYS(event_ring);
    seg->size = XHCI_EVRING_LEN;
    return 0;
}

/*
 * Submit a command by pushing a TRB to the
 * command ring.
 *
 * @hc: Host controller descriptor.
 * @trb: Transfer Request Block of command.
 */
static int
xhci_submit_cmd(struct xhci_hc *hc, struct xhci_trb trb)
{
    volatile uint32_t *cmd_db;
    struct xhci_caps *caps = XHCI_CAPS(hc->base);

    /* Push the TRB to the command ring */
    hc->cmd_ring[hc->cmd_ptr++] = trb.dword0;
    hc->cmd_ring[hc->cmd_ptr++] = trb.dword1;
    hc->cmd_ring[hc->cmd_ptr++] = trb.dword2;
    hc->cmd_ring[hc->cmd_ptr++] = trb.dword3 | hc->cycle;
    hc->cmd_count++;

    /* Ring the command doorbell */
    cmd_db = XHCI_CMD_DB(hc->base, caps->dboff);
    *cmd_db = 0;

    if (hc->cmd_count >= XHCI_CMDRING_LEN - 1) {
        /*
         * Create raw link TRB and ring the doorbell. We want the
         * xHC to flip its cycle bit so it doesn't confuse existing
         * entries (that we'll overwrite) in the ring with current
         * entries, so we set the Toggle Cycle bit.
         *
         * See the xHCI spec, section 6.4.4.1 for information regarding
         * the format of link TRBs.
         */
        hc->cmd_ring[hc->cmd_ptr++] = VIRT_TO_PHYS(hc->cmd_ring) & 0xFFFFFFFF;
        hc->cmd_ring[hc->cmd_ptr++] = VIRT_TO_PHYS(hc->cmd_ring) >> 32;
        hc->cmd_ring[hc->cmd_ptr++] = 0;
        hc->cmd_ring[hc->cmd_ptr++] = hc->cycle | (XHCI_LINK << 10) | __BIT(1);
        *cmd_db = 0;

        /* Reset command state and flip cycle */
        hc->cmd_ptr = 0;
        hc->cmd_count = 0;
        hc->cycle = ~hc->cycle;
    }

    return 0;
}

/*
 * Parse xHCI extended caps
 */
static int
xhci_parse_ecp(struct xhci_hc *hc)
{
    struct xhci_caps *caps = XHCI_CAPS(hc->base);
    struct xhci_proto *proto;
    uint32_t *p, val, dword2;
    uint32_t cap_ptr = XHCI_ECP(caps->hccparams1);

    p = XHCI_BASE_OFF(hc->base, cap_ptr*4);
    while (cap_ptr != 0) {
        val = *p;
        dword2 = *((uint32_t *)XHCI_BASE_OFF(p, 8));

        /* Get the next cap */
        p = XHCI_BASE_OFF(p, XHCI_PROTO_NEXT(val) * 4);
        cap_ptr = XHCI_PROTO_NEXT(val);

        if (XHCI_PROTO_ID(val) != XHCI_ECAP_PROTO) {
            /* Not a Supported Protocol Capability */
            continue;
        }

        if (hc->protocnt > XHCI_MAX_PROTOS) {
            /* Too many protocols */
            break;
        }

        proto = &hc->protos[hc->protocnt++];
        proto->major = XHCI_PROTO_MAJOR(val);
        proto->port_count = XHCI_PROTO_PORTCNT(dword2);
        proto->port_start = XHCI_PROTO_PORTOFF(dword2);
    }

    return 0;
}

/*
 * Set of xHCI scratchpad buffers.
 */
static int
xhci_init_scratchpads(struct xhci_hc *hc)
{
    struct xhci_caps *caps = XHCI_CAPS(hc->base);
    uint16_t max_bufs_lo, max_bufs_hi, max_bufs;
    uintptr_t buffer_array;

    max_bufs_lo = XHCI_MAX_SP_LO(caps->hcsparams2);
    max_bufs_hi = XHCI_MAX_SP_HI(caps->hcsparams2);
    max_bufs = (max_bufs_hi << 5) | max_bufs_lo;
    if (max_bufs == 0) {
        /*
         * Some emulators like QEMU don't need any
         * scratchpad buffers so we can just return
         * early.
         */
        return 0;
    }

    KINFO("HC requires %d max scratchpad buffers(s)\n", max_bufs);
    buffer_array = vm_alloc_pageframe(max_bufs);
    if (buffer_array == 0) {
        KERR("Failed to allocate scratchpad buffer(s)\n");
        return -1;
    }

    vm_zero_page(PHYS_TO_VIRT(buffer_array), max_bufs);
    hc->dcbaap[0] = buffer_array;
    return 0;
}

/*
 * Init USB ports on the root hub.
 */
static int
xhci_init_ports(struct xhci_hc *hc)
{
    struct xhci_caps *caps = XHCI_CAPS(hc->base);
    size_t maxports = XHCI_MAXPORTS(caps->hcsparams1);
    uint32_t *portsc;

    for (size_t i = 1; i < maxports; ++i) {
        portsc = xhci_get_portsc(hc, i);
        if (__TEST(*portsc, XHCI_PORTSC_CCS)) {
            KINFO("Device connected on port %d, resetting...\n", i);
            *portsc |= XHCI_PORTSC_PR;
        }
    }

    return 0;
}

/*
 * Poll USBSTS.HCH to be val
 *
 * Returns 0 on success. Non-zero values returned
 * indicate a hang.
 */
static int
xhci_poll_hch(struct xhci_hc *hc, uint8_t val)
{
    struct xhci_opregs *opregs = hc->opregs;
    size_t time_waiting = 0;

    while ((opregs->usbsts & USBSTS_HCH) != val) {
        if (time_waiting >= XHCI_TIMEOUT) {
            /* Hang */
            return -1;
        }

        time_waiting += 50;
        driver_tmr.msleep(50);
    }

    return 0;
}

/*
 * Start up the host controller by setting
 * the USBCMD run/stop bit.
 */
static int
xhci_start_hc(struct xhci_hc *hc)
{
    struct xhci_opregs *opregs = hc->opregs;
    int status;

    opregs->usbcmd |= USBCMD_RUN;

    if ((status = xhci_poll_hch(hc, 0)) != 0) {
        return status;
    }

    return 0;
}

static int
xhci_reset_hc(struct xhci_hc *hc)
{
    struct xhci_opregs *opregs = hc->opregs;
    size_t time_waiting = 0;    /* In ms */

    KINFO("Resetting host controller...\n");

    /*
     * Set USBCMD.HCRST to reset the controller and
     * wait for it to become zero.
     */
    opregs->usbcmd |= USBCMD_HCRST;
    while (1) {
        if (!__TEST(opregs->usbcmd, USBCMD_HCRST)) {
            /* Reset is complete */
            break;
        }
        if (time_waiting >= XHCI_TIMEOUT) {
            KERR("Hang while polling USBCMD.HCRST to be zero\n");
            return -1;
        }
        driver_tmr.msleep(50);
        time_waiting += 50;
    }
    return 0;
}

/*
 * Allocate the Device Context Base Address
 * Array.
 *
 * Returns the physical address and sets
 * hc->dcbaap to the virtual address.
 */
static uintptr_t
xhci_alloc_dcbaa(struct xhci_hc *hc)
{
    size_t dcbaa_size;

    dcbaa_size = sizeof(uintptr_t) * hc->maxslots;
    hc->dcbaap = dynalloc_memalign(dcbaa_size, 0x1000);
    __assert(hc->dcbaap != NULL);

    return VIRT_TO_PHYS(hc->dcbaap);
}

/*
 * Allocates command ring and sets hc->cmd_ring
 * to the virtual address.
 *
 * Returns the physical address.
 */
static uintptr_t
xhci_alloc_cmdring(struct xhci_hc *hc)
{
    size_t cmdring_size;

    cmdring_size = XHCI_TRB_SIZE * XHCI_CMDRING_LEN;
    hc->cmd_ring = dynalloc_memalign(cmdring_size, 0x1000);
    __assert(hc->cmd_ring != NULL);

    return VIRT_TO_PHYS(hc->cmd_ring);
}

/*
 * Allocates the event ring segment
 * and sets hc->evring_seg to the virtual address.
 *
 * Returns the physical address.
 */
static uintptr_t
xhci_alloc_evring(struct xhci_hc *hc)
{
    size_t evring_segment_sz;

    /* Allocate event ring segment */
    evring_segment_sz = sizeof(struct xhci_evring_segment);
    hc->evring_seg = dynalloc_memalign(evring_segment_sz, 0x1000);
    xhci_init_evring_segment(hc->evring_seg);

    return VIRT_TO_PHYS(hc->evring_seg);
}

static int
xhci_init_hc(struct xhci_hc *hc)
{
    struct xhci_caps *caps;
    struct xhci_opregs *opregs;

    /* Get some information from the controller */
    caps = XHCI_CAPS(hc->base);
    hc->caplen = caps->caplength;
    hc->maxslots = XHCI_MAXSLOTS(caps->hcsparams1);
    hc->maxports = XHCI_MAXSLOTS(caps->hcsparams1);

    opregs->config |= hc->maxslots;

    /* Fetch the opregs */
    opregs = XHCI_OPBASE(hc->base, hc->caplen);
    hc->opregs = XHCI_OPBASE(hc->base, hc->caplen);

    if (xhci_reset_hc(hc) != 0) {
        return -1;
    }

    /* Set cmdring state */
    hc->cycle = 1;
    hc->cmd_ptr = 0;
    hc->cmd_count = 0;

    /* Allocate resources and tell the HC about them */
    opregs->dcbaa_ptr = xhci_alloc_dcbaa(hc);
    xhci_init_scratchpads(hc);
    opregs->cmd_ring = xhci_alloc_cmdring(hc);
    xhci_set_erst_base(hc, xhci_alloc_evring(hc));

    /* We're ready, start up the HC and ports */
    xhci_start_hc(hc);
    xhci_parse_ecp(hc);
    xhci_init_ports(hc);
    return 0;
}

static int
xhci_init(void)
{
    uintptr_t bar0, bar1, base;
    struct xhci_hc hc;
    struct pci_lookup hc_lookup = {
        .pci_class = 0x0C,
        .pci_subclass = 0x03
    };

    /* Find the host controller on the bus */
    hci_dev = pci_get_device(hc_lookup, PCI_CLASS | PCI_SUBCLASS);
    if (hci_dev == NULL) {
        return -1;
    }

    bar0 = hci_dev->bar[0] & ~7;
    bar1 = hci_dev->bar[1] & ~7;
    base = __COMBINE32(bar1, bar0);
    hc.base = PHYS_TO_VIRT(base);
    KINFO("xHCI HC base @ 0x%p\n", base);

    if (req_timer(TIMER_GP, &driver_tmr) != 0) {
        KERR("Failed to fetch general purpose timer\n");
        return -1;
    }
    if (driver_tmr.msleep == NULL) {
        KERR("Timer does not have msleep()\n");
        return -1;
    }

    return xhci_init_hc(&hc);
}

DRIVER_EXPORT(xhci_init);

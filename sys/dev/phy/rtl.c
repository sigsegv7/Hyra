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
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/spinlock.h>
#include <sys/driver.h>
#include <sys/device.h>
#include <dev/pci/pci.h>
#include <dev/phy/rtl.h>
#include <dev/timer.h>
#include <dev/pci/pciregs.h>
#include <net/netbuf.h>
#include <net/if_var.h>
#include <vm/physmem.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <string.h>

#define IFNAME "rt0"

#define pr_trace(fmt, ...) kprintf("rt81xx: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define RX_BUF_SIZE      3      /* In pages */
#define RX_REAL_BUF_SIZE 8192   /* In bytes */
#define TXQ_ENTRIES 4

#define RX_PTR_MASK (~3)

/* Does our platform support PIO? */
#if defined(_MACHINE_HAVE_PIO) || defined(__x86_64__)
#define HAVE_PIO 1
#else
#define HAVE_PIO 0
#endif  /* _MACHINE_HAVE_PIO */

static struct spinlock netif_lock;
static struct netbuf netif_buf[TXQ_ENTRIES];
static struct pci_device *dev;
static struct netif netif;
static struct timer tmr;
static uint32_t tx_ptr = 0;
static uint32_t netif_enq_ptr = 0;
static uint16_t ioport;
static paddr_t rxbuf, txbuf;

/* TXAD regs */
static uint16_t tsads[TXQ_ENTRIES] = {
    RT_TXAD_N(0), RT_TXAD_N(4),
    RT_TXAD_N(8), RT_TXAD_N(12)
};
static uint16_t tsds[TXQ_ENTRIES] = {
    RT_TXSTATUS_N(0), RT_TXSTATUS_N(4),
    RT_TXSTATUS_N(8), RT_TXSTATUS_N(8)
};

/*
 * Write to an RTL8139 register
 *
 * @size: 1 (8-bits), 2 (16-bits), or 4 (32 bits)
 */
static inline void
rt_write(uint8_t reg, uint8_t size, uint32_t val)
{
    switch (size) {
    case 1:
        outb(ioport + reg, (uint8_t)val);
        break;
    case 2:
        outw(ioport + reg, (uint16_t)val);
        break;
    case 4:
        outl(ioport + reg, val);
        break;
    default:
        pr_error("rt_write: bad size %d\n", size);
        break;
    }
}

/*
 * Read from an RTL8139 register
 *
 * @size: 1 (8-bits), 2 (16-bits), or 4 (32 bits)
 */
static inline uint32_t
rt_read(uint8_t reg, uint8_t size)
{
    uint32_t retval;

    switch (size) {
    case 1:
        retval = inb(ioport + reg);
        break;
    case 2:
        retval = inw(ioport + reg);
        break;
    case 4:
        retval = inl(ioport + reg);
        break;
    default:
        pr_error("rt_read: bad size %d\n", size);
        return MASK(sizeof(retval) * 8);
    }

    return retval;
}

/*
 * Poll register to have 'bits' set/unset.
 *
 * @reg: Register to poll.
 * @bits: Bits to be checked.
 * @pollset: True to poll as set.
 */
static uint32_t
rt_poll(uint8_t reg, uint8_t size, uint32_t bits, bool pollset)
{
    size_t usec_start, usec;
    size_t elapsed_msec;
    uint32_t val;
    bool tmp;

    usec_start = tmr.get_time_usec();
    for (;;) {
        val = rt_read(reg, size);
        tmp = (pollset) ? ISSET(val, bits) : !ISSET(val, bits);

        usec = tmr.get_time_usec();
        elapsed_msec = (usec - usec_start) / 1000;

        /* If tmp is set, the register updated in time */
        if (tmp) {
            break;
        }

        /* Exit with an error if we timeout */
        if (elapsed_msec > RT_TIMEOUT_MSEC) {
            return -ETIME;
        }
    }
    return val;
}

static int
rt_tx(void *packet, size_t len)
{
    static uint32_t tx_ptr = 0;
    void *tx_data;
    paddr_t tx_pa;

    tx_data = dynalloc(len);
    if (tx_data == NULL) {
        return -ENOMEM;
    }

    memcpy(tx_data, packet, len);
    tx_pa = VIRT_TO_PHYS(tx_data);
    rt_write(tsads[tx_ptr], 4, tx_pa);
    rt_write(tsds[tx_ptr++], 4, len);
    if (tx_ptr > TXQ_ENTRIES - 1) {
        tx_ptr = 0;
    }
    return 0;
}

static void
__rt81xx_tx_start(struct netif *nifp)
{
    struct netbuf *dest;
    int error;

    for (int i = 0; i < netif_enq_ptr; ++i) {
        dest = &netif_buf[i];
        error = rt_tx(dest->data, dest->len);
        if (error < 0) {
            pr_error("tx_start fail @queue %d (errno=%d)\n", i, error);
        }
    }
}

static void
rt81xx_tx_start(struct netif *nifp)
{
    spinlock_acquire(&netif_lock);
    __rt81xx_tx_start(nifp);
    spinlock_release(&netif_lock);
}

static int
rt81xx_tx_enq(struct netif *nifp, struct netbuf *nbp, void *data)
{
    struct netbuf *dest;

    spinlock_acquire(&netif_lock);
    dest = &netif_buf[netif_enq_ptr++];
    memcpy(dest, nbp, sizeof(*dest));

    if (netif_enq_ptr > TXQ_ENTRIES - 1) {
        __rt81xx_tx_start(nifp);
        netif_enq_ptr = 0;
    }
    spinlock_release(&netif_lock);
    return 0;
}

static int
rt81xx_intr(void *sp)
{
    uint16_t len;
    uint16_t *p;
    uint16_t status;

    status = rt_read(RT_INTRSTATUS, 2);
    p = (uint16_t *)(rxbuf + tx_ptr);
    len = *(p + 1);     /* Length after header */
    p += 2;             /* Points to data now */

    if (!ISSET(status, RT_TOK | RT_ROK)) {
        return 0;
    }

    if (ISSET(status, RT_TOK)) {
        pr_trace("sent packet\n");
        return 1;
    }

    /* Update rxbuf offset in CAPR */
    tx_ptr = (tx_ptr + len + 4 + 3) & RX_PTR_MASK;
    if (tx_ptr > RX_REAL_BUF_SIZE) {
        tx_ptr -= RX_REAL_BUF_SIZE;
    }
    rt_write(RT_RXBUFTAIL, 2, tx_ptr - 0x10);
    rt_write(RT_INTRSTATUS, 2, RT_ACKW);
    return 1;       /* handled */
}

static int
rt81xx_irq_init(void)
{
    struct intr_hand ih;

    ih.func = rt81xx_intr;
    ih.priority = IPL_BIO;
    ih.irq = dev->irq_line;
    if (intr_register("rt81xx", &ih) == NULL) {
        return -EIO;
    }
    return 0;
}

static void
rt_init_pci(void)
{
    uint32_t tmp;

    /* Enable bus mastering and MMIO */
    tmp = pci_readl(dev, PCIREG_CMDSTATUS);
    tmp |= (PCI_BUS_MASTERING | PCI_MEM_SPACE);
    pci_writel(dev, PCIREG_CMDSTATUS, tmp);
}

static int
rt_init_mac(void)
{
    struct netif_addr *addr = &netif.addr;
    uint8_t conf;
    uint32_t tmp;
    int error;

    /*
     * First step is ensuring the MAC is in known
     * and consistent state by resetting it. God
     * knows what BIOS or UEFI did to it...
     */
    ioport = dev->bar[0] & ~1;
    pr_trace("resetting MAC...\n");
    rt_write(RT_CHIPCMD, 1, RT_RST);
    error = rt_poll(RT_CHIPCMD, 1, RT_RST, 0);
    if (error < 0) {
        pr_error("RTL8139 reset timeout\n");
        return error;
    }

    /*
     * Tell the RTL8139 to load config data from
     * the 93C46. This is done by clearing EEM1
     * and setting EEM0. This whole process should
     * take roughly 2 milliseconds.
     *
     * XXX: EEPROM autoloads *should* happen during a hardware
     *      reset but some cards might not follow spec so force
     *      it.
     */
    conf = rt_read(RT_CFG9346, 1);
    conf &= ~RT_EEM1;
    conf |= RT_EEM0;
    rt_write(RT_CFG9346, 1, conf);

    /* MAC address dword 0 */
    tmp = rt_read(RT_IDR0, 4);
    addr->data[0] = tmp & 0xFF;
    addr->data[1] = (tmp >> 8) & 0xFF;
    addr->data[2] = (tmp >> 16) & 0xFF;
    addr->data[3] = (tmp >> 24) & 0xFF;

    /* MAC address word 1 */
    tmp = rt_read(RT_IDR2, 4);
    addr->data[4] = (tmp >> 16) & 0xFF;
    addr->data[5] = (tmp >> 24) & 0xFF;

    pr_trace("MAC address: %x:%x:%x:%x:%x:%x\n",
        (uint64_t)addr->data[0], (uint64_t)addr->data[1],
        (uint64_t)addr->data[2], (uint64_t)addr->data[3],
        (uint64_t)addr->data[4], (uint64_t)addr->data[5]);

    /*
     * Alright, now we don't want those EEM bits
     * sticking lopsided so lets put the RTL8139
     * back into normal operation...
     */
    conf = rt_read(RT_CFG9346, 1);
    conf &= ~(RT_EEM1 | RT_EEM0);
    rt_write(RT_CFG9346, 1, conf);

    rxbuf = vm_alloc_frame(RX_BUF_SIZE);
    txbuf = vm_alloc_frame(RX_BUF_SIZE);

    if (rxbuf == 0 || txbuf == 0) {
        if (rxbuf != 0)
            vm_free_frame(rxbuf, RX_BUF_SIZE);
        if (txbuf != 0)
            vm_free_frame(txbuf, RX_BUF_SIZE);

        pr_error("failed to alloc TX/RX buffers\n");
        return -ENOMEM;
    }

    memcpy(netif.name, IFNAME, strlen(IFNAME) + 1);
    netif.tx_enq = rt81xx_tx_enq;
    netif.tx_start = rt81xx_tx_start;
    netif_add(&netif);

    /*
     * Configure the chip:
     *
     * - Enable machdep IRQ
     * - Point RX buffer
     * - Setup RX buffer flags:
     *     * Accept broadcast
     *     * Accept multicast
     *     * Accept physical match
     *     * Accept all packets (promiscuous mode)
     *     ? (AB/AM/APM/AAP)
     *
     * TODO: ^ Some of these should be configurable ^
     *
     * - Enable interrupts through ROK/TOK
     * - Enable RX state machines
     *
     */
    rt81xx_irq_init();
    rt_write(RT_RXBUF, 4, rxbuf);
    rt_write(RT_RXCONFIG, 4, RT_AB | RT_AM | RT_APM | RT_AAP);
    rt_write(RT_INTRMASK, 2, RT_ROK | RT_TOK);
    rt_write(RT_CHIPCMD, 1, RT_RE | RT_TE);
    return 0;
}

static int
rt81xx_init(void)
{
    struct pci_lookup lookup;

    lookup.vendor_id = 0x10EC;
    lookup.device_id = 0x8139;
    dev = pci_get_device(lookup, PCI_VENDOR_ID | PCI_DEVICE_ID);
    if (dev == NULL) {
        return -ENODEV;
    }

    pr_trace("Realtek network ctrl <phy? at pci%d:%x.%x.%d>\n",
        dev->bus, dev->device_id, dev->func,
        dev->slot);

    if (HAVE_PIO == 0) {
        pr_error("port i/o not supported, bailing\n");
        return -ENOTSUP;
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

    /* We also need msleep() */
    if (tmr.msleep == NULL) {
        pr_error("general purpose timer has no msleep()\n");
        return -ENODEV;
    }

    rt_init_pci();
    return rt_init_mac();
}

DRIVER_DEFER(rt81xx_init, "rtl81xx");

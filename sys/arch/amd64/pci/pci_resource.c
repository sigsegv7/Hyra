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

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <dev/pci/resource.h>
#include <dev/pci/pci.h>
#include <dev/pci/pciregs.h>

// #define DEBUG 1

#define pr_trace(fmt, ...) kprintf("pci: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#if defined(DEBUG)
#define pr_debug(...) pr_trace(__VA_ARGS__)
#else
#define pr_debug(...) __nothing
#endif

/*
 * Enables bus mastering for a specific PCI device.
 *
 * @brp: Pointer to bus resource handle.
 * @devp: Pointer to `pci_device' structure.
 *
 * XXX: If the bus resource is MMIO capable, PCI
 *      memory space access will become set within
 *      the command/status register
 */
int
pcir_enable_dma(struct bus_resource *brp, void *devp)
{
    struct pci_device *dev = devp;
    uint32_t tmp;

    if (brp == NULL || dev == NULL) {
        return -EINVAL;
    }

    /*
     * Make sure that we are allowed to perform DMA. This
     * will fail if the bus isn't DMA-capable or if DMA
     * is disabled.
     */
    if (!ISSET(brp->sem, BUS_DMA)) {
        pr_trace("Bus marked non DMA capable, DMA not enabled\n");
        return -EACCES;
    }

    tmp = pci_readl(dev, PCIREG_CMDSTATUS);
    tmp |= PCI_BUS_MASTERING;
    tmp |= ISSET(brp->sem, BUS_MMIO);
    pci_writel(dev, PCIREG_CMDSTATUS, tmp);
    return 0;
}

/*
 * Disables bus mastering for a specific PCI device.
 *
 * @brp: Pointer to bus resource handle.
 * @devp: Pointer to `pci_device' structure.
 *
 * XXX: If the bus resource is not MMIO capable, PCI
 *      memory space access will become unset within
 *      the command/status register
 */
int
pcir_disable_dma(struct bus_resource *brp, void *devp)
{
    struct pci_device *dev = devp;
    uint32_t tmp;

    if (brp == NULL || dev == NULL) {
        return -EINVAL;
    }

    tmp = pci_readl(dev, PCIREG_CMDSTATUS);
    tmp &= ~PCI_BUS_MASTERING;
    tmp &= ~ISSET(brp->sem, BUS_MMIO);
    pci_writel(dev, PCIREG_CMDSTATUS, tmp);
    return 0;
}

int
pcir_set_sem(struct bus_resource *brp, bus_sem_t sem)
{
    /*
     * BUS_PIO and BUS_MMIO are two different things
     * and must be mutually exclusive...
     */
    if (ISSET(sem, BUS_MMIO) && ISSET(sem, BUS_PIO)) {
        pr_debug("Bad semantics (sem=%x)\n", sem);
        return -EINVAL;
    }

    brp->sem |= sem;
    return 0;
}

int
pcir_clr_sem(struct bus_resource *brp, bus_sem_t sem)
{
    brp->sem &= ~sem;
    return 0;
}

__unused int
pcir_dma_alloc(struct bus_resource *brp, void *res)
{
    return 0;
}

__unused int
pcir_dma_free(struct bus_resource *brp, void *p)
{
    return 0;
}

ssize_t
pcir_dma_in(struct bus_resource *brp, void *p)
{
    return 0;
}

ssize_t
pcir_dma_out(struct bus_resource *brp, void *p)
{
    return 0;
}

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
#include <sys/sched.h>
#include <sys/syslog.h>
#include <sys/mmio.h>
#include <sys/device.h>
#include <fs/devfs.h>
#include <dev/ic/nvmeregs.h>
#include <dev/ic/nvmevar.h>
#include <dev/pci/pci.h>
#include <dev/pci/pciregs.h>
#include <dev/timer.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("nvme: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static struct bdevsw nvme_bdevsw;
static TAILQ_HEAD(,nvme_ns) namespaces;
static struct pci_device *nvme_dev;
static struct timer tmr;

static int nvme_poll_submit_cmd(struct nvme_queue *q, struct nvme_cmd cmd);

static inline int
is_4k_aligned(void *ptr)
{
    return ((uintptr_t)ptr & (0x1000 - 1)) == 0;
}

/*
 * Fetch a namespace from its device ID
 *
 * @dev: Device ID of namespace to fetch.
 */
static struct nvme_ns *
nvme_get_ns(dev_t dev)
{
    struct nvme_ns *ns;

    TAILQ_FOREACH(ns, &namespaces, link) {
        if (ns->dev == dev) {
            return ns;
        }
    }

    return NULL;
}

/*
 * Poll register to have 'bits' set/unset.
 *
 * @reg: Register to poll.
 * @bits: Bits to be checked.
 * @pollset: True to poll as set.
 */
static int
nvme_poll_reg(struct nvme_bar *bar, volatile uint32_t *reg, uint32_t bits,
              bool pollset)
{
    size_t usec_start, usec;
    size_t elapsed_msec;
    uint32_t val, caps;
    bool tmp;

    usec_start = tmr.get_time_usec();
    caps = mmio_read32(&bar->caps);

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
        if (elapsed_msec > CAP_TIMEOUT(caps)) {
            return -ETIME;
        }
    }

    return val;
}

static int
nvme_create_queue(struct nvme_bar *bar, struct nvme_queue *queue, size_t id)
{
    uint8_t dbstride;
    uint16_t slots;
    uint64_t  caps;
    uintptr_t sq_db, cq_db;

    caps = mmio_read32(&bar->caps);
    dbstride = CAP_STRIDE(caps);
    slots = CAP_MQES(caps);

    queue->sq = dynalloc_memalign(sizeof(void *) * slots, 0x1000);
    queue->cq = dynalloc_memalign(sizeof(void *) * slots, 0x1000);

    if (queue->sq == NULL) {
        return -ENOMEM;
    }

    if (queue->cq == NULL) {
        dynfree(queue->sq);
        return -ENOMEM;
    }

    memset(queue->sq, 0, sizeof(void *) * slots);
    memset(queue->cq, 0, sizeof(void *) * slots);

    sq_db = (uintptr_t)bar + DEFAULT_PAGESIZE + (2 * id * (4 << dbstride));
    cq_db = (uintptr_t)bar + DEFAULT_PAGESIZE + ((2 * id + 1) * (4 << dbstride));

    queue->sq_head = 0;
    queue->sq_tail = 0;

    queue->size = slots;
    queue->cq_phase = 1;
    queue->sq_db = (void *)sq_db;
    queue->cq_db = (void *)cq_db;
    return 0;
}

static int
nvme_create_ioq(struct nvme_ns *ns, size_t id)
{
    struct nvme_queue *ioq = &ns->ioq;
    struct nvme_ctrl *ctrl = ns->ctrl;
    struct nvme_bar *bar = ctrl->bar;
    struct nvme_create_iocq_cmd *create_iocq;
    struct nvme_create_iosq_cmd *create_iosq;
    struct nvme_cmd cmd = {0};
    uint32_t caps;
    uint16_t mqes;
    int error;

    caps = mmio_read32(&bar->caps);
    mqes = CAP_MQES(caps);

    if ((error = nvme_create_queue(bar, ioq, id)) != 0)
        return error;

    create_iocq = &cmd.create_iocq;
    create_iocq->opcode = NVME_OP_CREATE_IOCQ;
    create_iocq->qflags = BIT(0);   /* Physically contiguous */
    create_iocq->qsize = mqes;
    create_iocq->qid = id;
    create_iocq->prp1 = VIRT_TO_PHYS(ns->ioq.cq);

    if ((error = nvme_poll_submit_cmd(&ctrl->adminq, cmd)) != 0)
        return error;

    create_iosq = &cmd.create_iosq;
    create_iosq->opcode = NVME_OP_CREATE_IOSQ;
    create_iosq->qflags = BIT(0);    /* Physically contiguous */
    create_iosq->qsize = mqes;
    create_iosq->cqid = id;
    create_iosq->sqid = id;
    create_iosq->prp1 = VIRT_TO_PHYS(ns->ioq.sq);
    return nvme_poll_submit_cmd(&ctrl->adminq, cmd);
}

/*
 * Stop and reset the NVMe controller.
 */
static int
nvme_stop_ctrl(struct nvme_bar *bar)
{
    uint32_t config, status;

    /* Do not reset if CSTS.RDY is 0 */
    status = mmio_read32(&bar->status);
    if (!ISSET(status, STATUS_RDY)) {
        return 0;
    }

    /* Clear the enable bit to begin the reset */
    config = mmio_read32(&bar->config);
    config &= ~CONFIG_EN;
    mmio_write32(&bar->config, config);

    if (nvme_poll_reg(bar, &bar->status, STATUS_RDY, false) < 0) {
        pr_error("controller reset timeout\n");
        return -ETIME;
    }

    return 0;
}

/*
 * Start up the controller.
 */
static int
nvme_start_ctrl(struct nvme_bar *bar)
{
    uint32_t config, status;

    /* Cannot start if already started */
    status = mmio_read32(&bar->status);
    if (ISSET(status, STATUS_RDY)) {
        return 0;
    }

    /* Enable the controller */
    config = mmio_read32(&bar->config);
    config |= CONFIG_EN;
    mmio_write32(&bar->config, config);

    if (nvme_poll_reg(bar, &bar->status, STATUS_RDY, true) < 0) {
        pr_error("controller startup timeout\n");
        return -ETIME;
    }

    return 0;
}

/*
 * Submit a command.
 */
static void
nvme_submit_cmd(struct nvme_queue *q, struct nvme_cmd cmd)
{
    q->sq[q->sq_tail++] = cmd;
    if (q->sq_tail >= q->size) {
        q->sq_tail = 0;
    }

    mmio_write32(q->sq_db, q->sq_tail);
}

/*
 * Submit a command and poll for completion.
 */
static int
nvme_poll_submit_cmd(struct nvme_queue *q, struct nvme_cmd cmd)
{
    uint16_t status;
    uint8_t spins = 0;

    nvme_submit_cmd(q, cmd);

    for (;;) {
        tmr.msleep(100);

        /*
         * If the phase bit matches the most recently submitted
         * command then the command has completed
         */
        status = q->cq[q->cq_head].status;
        if ((status & 1) == q->cq_phase) {
            break;
        }

        /* Check for timeout */
        if (spins > 5) {
            pr_error("hang while polling phase bit, giving up\n");
            return -ETIME;
        }

        ++spins;
    }

    ++q->cq_head;
    if (q->cq_head >= q->size) {
        q->cq_head = 0;
        q->cq_phase = !q->cq_phase;
    }

    mmio_write32(q->cq_db, q->cq_head);
    return 0;
}

static int
nvme_identify(struct nvme_ctrl *ctrl, void *buf, uint32_t nsid, uint8_t cns)
{
    struct nvme_cmd cmd = {0};
    struct nvme_identify_cmd *idcmd = &cmd.identify;

    if (!is_4k_aligned(buf)) {
        return -1;
    }

    idcmd->opcode = NVME_OP_IDENTIFY;
    idcmd->nsid = nsid;
    idcmd->cns = cns;  /* Identify controller */
    idcmd->prp1 = VIRT_TO_PHYS(buf);
    idcmd->prp2 = 0;
    return nvme_poll_submit_cmd(&ctrl->adminq, cmd);
}

/*
 * For debugging purposes, logs some information
 * found within the controller identify data structure.
 */
static void
nvme_log_ctrl_id(struct nvme_id *id)
{
    char mn[41] = {0};
    char sn[21] = {0};
    char fr[9] = {0};

    for (size_t i = 0; i < sizeof(id->mn); ++i) {
        mn[i] = id->mn[i];
    }

    for (size_t i = 0; i < sizeof(id->fr); ++i) {
        fr[i] = id->fr[i];
    }

    for (size_t i = 0; i < sizeof(id->sn); ++i) {
        sn[i] = id->sn[i];
    }

    pr_trace("model number: %s\n", mn);
    pr_trace("serial number: %s\n", sn);
    pr_trace("firmware revision: %s\n", fr);
}

/*
 * Init PCI related controller bits
 */
static void
nvme_init_pci(void)
{
    uint32_t tmp;

    /* Enable bus mastering and MMIO */
    tmp = pci_readl(nvme_dev, PCIREG_CMDSTATUS);
    tmp |= (PCI_BUS_MASTERING | PCI_MEM_SPACE);
    pci_writel(nvme_dev, PCIREG_CMDSTATUS, tmp);
}

/*
 * Issue a read/write command for a specific
 * namespace.
 *
 * `buf' must be 4k aligned.
 */
static int
nvme_rw(struct nvme_ns *ns, char *buf, off_t slba, size_t count, bool write)
{
    struct nvme_cmd cmd = {0};
    struct nvme_rw_cmd *rw = &cmd.rw;

    if (!is_4k_aligned(buf)) {
        return -1;
    }

    rw->opcode = write ? NVME_OP_WRITE : NVME_OP_READ;
    rw->nsid = ns->nsid;
    rw->slba = slba;
    rw->len = count - 1;
    rw->prp1 = VIRT_TO_PHYS(buf);
    return nvme_poll_submit_cmd(&ns->ioq, cmd);
}

/*
 * Device interface read/write helper.
 *
 * @dev: Device ID.
 * @sio: SIO transaction descriptor.
 * @write: True if this is a write operation.
 *
 * This routine uses an internal buffer aligned on a
 * 4 KiB boundary to enable flexibility with the input
 * SIO buffer. This allows the SIO buffer to be unaligned
 * and/or sized smaller than the namespace block size.
 */
static int
nvme_dev_rw(dev_t dev, struct sio_txn *sio, bool write)
{
    struct nvme_ns *ns;
    size_t block_count, len;
    off_t block_off, read_off;
    int status;
    char *buf;

    if (sio == NULL)
        return -EINVAL;
    if (sio->len == 0 || sio->buf == NULL)
        return -EINVAL;

    /*
     * Get the NVMe namespace. This should not fail
     * but handle if it does just in case.
     */
    ns = nvme_get_ns(dev);
    if (__unlikely(ns == NULL))
        return -EIO;

    /* Calculate the block count and offset */
    block_count = ALIGN_UP(sio->len, ns->lba_bsize);
    block_count /= ns->lba_bsize;
    block_off = sio->offset / ns->lba_bsize;

    /* Allocate internal buffer */
    len = block_count * ns->lba_bsize;
    buf = dynalloc_memalign(len, 0x1000);
    if (buf == NULL)
        return -ENOMEM;

    /*
     * If this is a write, zero the internal buffer and copy over
     * the contents of the SIO buffer.
     */
    if (write) {
        memset(buf, 0, len);
        memcpy(buf, sio->buf, sio->len);
    }

    /*
     * Perform the r/w operation and copy internal buffer
     * out if this is a read operation.
     */
    status = nvme_rw(ns, buf, block_off, block_count, write);
    if (status == 0 && !write) {
        read_off = sio->offset & (ns->lba_bsize - 1);
        memcpy(sio->buf, buf + read_off, sio->len);
    }

    dynfree(buf);
    return status;
}

/*
 * Device interface read
 */
static int
nvme_dev_read(dev_t dev, struct sio_txn *sio, int flags)
{
    return nvme_dev_rw(dev, sio, false);
}

/*
 * Initializes an NVMe namespace.
 *
 * @ctrl: Controller.
 * @nsid: Namespace ID.
 */
static int
nvme_init_ns(struct nvme_ctrl *ctrl, uint8_t nsid)
{
    devmajor_t major;
    char devname[128];
    struct nvme_ns *ns = NULL;
    struct nvme_id_ns *idns = NULL;
    uint8_t lba_format;
    int status = 0;

    idns = dynalloc_memalign(sizeof(*idns), 0x1000);
    ns = dynalloc(sizeof(*ns));

    if (idns == NULL) {
        status = -ENOMEM;
        goto done;
    }

    if (ns == NULL) {
        status = -ENOMEM;
        goto done;
    }

    if ((status = nvme_identify(ctrl, idns, nsid, 0)) != 0)  {
        goto done;
    }

    /* Setup the namespace structure */
    lba_format = idns->flbas & 0xF;
    ns->lba_fmt = idns->lbaf[lba_format];
    ns->nsid = nsid;
    ns->lba_bsize = 1 << ns->lba_fmt.ds;
    ns->size = idns->size;
    ns->ctrl = ctrl;

    if ((status = nvme_create_ioq(ns, ns->nsid)) != 0) {
        goto done;
    }

    TAILQ_INSERT_TAIL(&namespaces, ns, link);
    snprintf(devname, sizeof(devname), "nvme0n%d", ns->nsid);

    /* Allocate major and minor */
    major = dev_alloc_major();
    ns->dev = dev_alloc(major);

    /* Register the namespace */
    dev_register(major, ns->dev, &nvme_bdevsw);
    devfs_create_entry(devname, major, ns->dev, 0444);
done:
    if (ns != NULL && status != 0)
        dynfree(ns);
    if (idns != NULL && status != 0)
        dynfree(idns);

    return status;
}

static int
nvme_init_ctrl(struct nvme_bar *bar)
{
    int error;
    uint64_t caps;
    uint32_t config;
    uint16_t mqes;
    uint8_t *nsids;
    struct nvme_ctrl ctrl = { .bar = bar };
    struct nvme_queue *adminq;
    struct nvme_id *id;

    /* Ensure the controller is stopped */
    if ((error = nvme_stop_ctrl(bar)) != 0) {
        return error;
    }

    adminq = &ctrl.adminq;
    caps = mmio_read64(&bar->caps);
    mqes = CAP_MQES(caps);

    /* Setup admin queues */
    nvme_create_queue(bar, adminq, 0);
    mmio_write32(&bar->aqa, (mqes | mqes << 16));
    mmio_write64(&bar->asq, VIRT_TO_PHYS(adminq->sq));
    mmio_write64(&bar->acq, VIRT_TO_PHYS(adminq->cq));

    /* Now bring the controller back up */
    if ((error = nvme_start_ctrl(bar)) != 0) {
        return error;
    }

    id = dynalloc_memalign(sizeof(*id), 0x1000);
    if (id == NULL) {
        return -ENOMEM;
    }

    nsids = dynalloc_memalign(0x1000, 0x1000);
    if (nsids == NULL) {
        dynfree(id);
        return -ENOMEM;
    }

    nvme_identify(&ctrl, id, 0, ID_CNS_CTRL);
    nvme_log_ctrl_id(id);
    nvme_identify(&ctrl, nsids, 0, ID_CNS_NSID_LIST);

    ctrl.sqes = id->sqes >> 4;
    ctrl.cqes = id->cqes >> 4;

    /*
     * Before creating any I/O queues we need to set CC.IOCQES
     * and CC.IOSQES... Bits 3:0 is the minimum and bits 7:4
     * is the maximum.
     */
    config = mmio_read32(&bar->config);
    config |= (ctrl.sqes << CONFIG_IOSQES_SHIFT);
    config |= (ctrl.cqes << CONFIG_IOCQES_SHIFT);
    mmio_write32(&bar->config, config);

    /* Init all active namespaces */
    for (size_t i = 0; i < id->nn; ++i) {
        if (nsids[i] == 0) {
            continue;
        }

        if (nvme_init_ns(&ctrl, nsids[i]) != 0) {
            pr_error("failed to initialize NSID %d\n", nsids[i]);
        }
    }

    dynfree(id);
    dynfree(nsids);
    return 0;
}

static int
nvme_init(void)
{
    struct pci_lookup lookup;
    struct nvme_bar *bar;
    int error;

    lookup.pci_class = 1;
    lookup.pci_subclass = 8;
    nvme_dev = pci_get_device(lookup, PCI_CLASS | PCI_SUBCLASS);

    if (nvme_dev == NULL) {
        return -ENODEV;
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

    TAILQ_INIT(&namespaces);
    nvme_init_pci();

    if ((error = pci_map_bar(nvme_dev, 0, (void *)&bar)) != 0) {
        return error;
    }

    return nvme_init_ctrl(bar);
}

static struct bdevsw nvme_bdevsw = {
    .read = nvme_dev_read,
    .write = nowrite
};

DRIVER_EXPORT(nvme_init);

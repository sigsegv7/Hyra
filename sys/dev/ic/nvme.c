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
#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <sys/timer.h>
#include <sys/device.h>
#include <dev/pci/pci.h>
#include <dev/ic/nvmevar.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <fs/devfs.h>
#include <string.h>

__MODULE_NAME("nvme");
__KERNEL_META("$Hyra$: nvme.c, Ian Marco Moffett, "
              "NVMe driver");

static struct pci_device *nvme_dev;
static struct timer driver_tmr;
static TAILQ_HEAD(,nvme_ns) namespaces;

static inline int
is_4k_aligned(void *ptr)
{
    return ((uintptr_t)ptr & (0x1000 - 1)) == 0;
}

/*
 * Poll CSTS.RDY to equal `val'
 *
 * Returns `val' on success, returns < 0 value
 * upon failure.
 */
static int
nvme_poll_ready(struct nvme_bar *bar, uint8_t val)
{
    uint8_t timeout = CAP_TIMEOUT(bar->caps);
    uint8_t time_waited = 0;

    do {
        if (STATUS_READY(bar->status) == val) {
            /* Done waiting */
            break;
        }

        /*
         * If CSTS.RDY hasn't changed, we can try to wait a
         * little longer.
         *
         * XXX: The spec states that CAP.TO (Timeout) is in 500
         *      millisecond units.
         */
        if (time_waited < timeout) {
            driver_tmr.msleep(500);
            ++time_waited;
        } else {
            return -1;
        }
    } while (1);

    return val;
}

/*
 * Create an NVMe queue.
 */
static int
nvme_create_queue(struct nvme_state *s, struct nvme_queue *queue, size_t id)
{
    struct nvme_bar *bar = s->bar;
    const size_t PAGESZ = vm_get_page_size();
    const uint8_t DBSTRIDE = CAP_STRIDE(bar->caps);
    const uint16_t SLOTS = CAP_MQES(bar->caps);

    queue->sq = dynalloc_memalign(sizeof(void *) * SLOTS, 0x1000);
    queue->cq = dynalloc_memalign(sizeof(void *) * SLOTS, 0x1000);

    if (queue->sq == NULL) {
        return -1;
    }
    if (queue->cq == NULL) {
        dynfree(queue->sq);
        return -1;
    }

    memset(queue->sq, 0, sizeof(void *) * SLOTS);
    memset(queue->cq, 0, sizeof(void *) * SLOTS);

    queue->sq_head = 0;
    queue->sq_tail = 0;
    queue->size = SLOTS;
    queue->sq_db = PHYS_TO_VIRT((uintptr_t)bar + PAGESZ + (2 * id * (4 << DBSTRIDE)));
    queue->cq_db = PHYS_TO_VIRT((uintptr_t)bar + PAGESZ + ((2 * id + 1) * (4 << DBSTRIDE)));
    queue->cq_phase = 1;
    return 0;
}

/*
 * Submit a command
 *
 * @queue: Target queue.
 * @cmd: Command to submit
 */
static void
nvme_submit_cmd(struct nvme_queue *queue, struct nvme_cmd cmd)
{
    /* Submit the command to the queue */
    queue->sq[queue->sq_tail++] = cmd;
    if (queue->sq_tail >= queue->size) {
        queue->sq_tail = 0;
    }
    *(queue->sq_db) = queue->sq_tail;
}

/*
 * Submit a command and poll for completion
 *
 * @queue: Target queue.
 * @cmd: Command to submit
 */
static int
nvme_poll_submit_cmd(struct nvme_queue *queue, struct nvme_cmd cmd)
{
    uint16_t status;
    size_t spins = 0;

    nvme_submit_cmd(queue, cmd);

    /*
     * Wait for the current command to complete by
     * polling the phase bit.
     */
    while (1) {
        status = queue->cq[queue->cq_head].status;
        if ((status & 1) == queue->cq_phase) {
            /*
             * The phase bit matches the phase for the most
             * recently submitted command, the command has completed.
             */
            break;
        }
        if ((status & ~1) != 0) {
            KDEBUG("NVMe cmd error (bits=0x%x)\n", status >> 1);
            break;
        }
        if (spins > 5) {
            /* Attempts exhausted */
            KERR("Hang on phase bit poll, giving up (cmd error)\n");
            break;
        }

        /* Not done, give it some more time */
        driver_tmr.msleep(150);
        ++spins;
    }

    ++queue->cq_head;
    if (queue->cq_head >= queue->size) {
        queue->cq_head = 0;
        queue->cq_phase = !queue->cq_phase;
    }

    /* Tell the controller that `head' updated */
    *(queue->cq_db) = queue->cq_head;
    return 0;
}

/*
 * Create an I/O queue for a specific namespace.
 *
 * @ns: Namespace
 * @id: I/O queue ID
 */
static int
nvme_create_ioq(struct nvme_ns *ns, size_t id)
{
    struct nvme_queue *ioq = &ns->ioq;
    struct nvme_state *cntl = ns->cntl;

    struct nvme_bar *bar = cntl->bar;
    struct nvme_cmd cmd = {0};
    size_t mqes = CAP_MQES(bar->caps);

    struct nvme_create_iocq_cmd *create_iocq;
    struct nvme_create_iosq_cmd *create_iosq;
    int status;

    if ((status = nvme_create_queue(ns->cntl, ioq, id)) != 0) {
        return status;
    }

    create_iocq = &cmd.create_iocq;
    create_iocq->opcode = NVME_OP_CREATE_IOCQ;
    create_iocq->qflags |= __BIT(0);    /* Physically contiguous */
    create_iocq->qsize = mqes;
    create_iocq->qid = id;
    create_iocq->prp1 = VIRT_TO_PHYS(ns->ioq.cq);

    if ((status = nvme_poll_submit_cmd(&cntl->adminq, cmd)) != 0) {
        return status;
    }

    create_iosq = &cmd.create_iosq;
    create_iosq->opcode = NVME_OP_CREATE_IOSQ;
    create_iosq->qflags |= __BIT(0);    /* Physically contiguous */
    create_iosq->qsize = mqes;
    create_iosq->cqid = id;
    create_iosq->sqid = id;
    create_iosq->prp1 = VIRT_TO_PHYS(ns->ioq.sq);
    return nvme_poll_submit_cmd(&cntl->adminq, cmd);
}

/*
 * Issue an identify command for the current
 * controller.
 *
 * XXX: `id' must be aligned on a 4k byte boundary to avoid
 *      crossing a page boundary. This keeps the implementation
 *      as simple as possible here.
 */
static int
nvme_identify(struct nvme_state *state, struct nvme_id *id)
{
    struct nvme_cmd cmd = {0};
    struct nvme_identify_cmd *identify = &cmd.identify;

    /* Ensure `id' is aligned on a 4k byte boundary */
    if (!is_4k_aligned(id)) {
        return -1;
    }

    identify->opcode = NVME_OP_IDENTIFY;
    identify->nsid = 0;
    identify->cns = 1;   /* Identify controller */
    identify->prp1 = VIRT_TO_PHYS(id);
    identify->prp2 = 0;  /* No need, data address is 4k aligned */
    return nvme_poll_submit_cmd(&state->adminq, cmd);
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
 * Fetch a namespace from its ID
 *
 * @nsid: Namespace ID of namespace to fetch
 */
static struct nvme_ns *
nvme_get_ns(size_t nsid)
{
    struct nvme_ns *ns;

    TAILQ_FOREACH(ns, &namespaces, link) {
        if (ns->nsid == nsid) {
            return ns;
        }
    }

    return NULL;
}

/*
 * Device interface read/write helper
 */
static int
nvme_dev_rw(struct device *dev, struct sio_txn *sio, bool write)
{
    struct nvme_ns *ns;

    if (sio == NULL) {
        return -1;
    }

    ns = nvme_get_ns(dev->minor);
    if (ns == NULL || sio->buf == NULL) {
        return -1;
    }

    return nvme_rw(ns, sio->buf, sio->offset, sio->len, write);
}

/*
 * Device interface read
 */
static int
nvme_dev_read(struct device *dev, struct sio_txn *sio)
{
    return nvme_dev_rw(dev, sio, false);
}

/*
 * Device interface write
 */
static int
nvme_dev_write(struct device *dev, struct sio_txn *sio)
{
    return nvme_dev_rw(dev, sio, true);
}

/*
 * Get identify data for namespace
 *
 * @id_ns: Data will be written to this pointer via DMA.
 * @nsid: Namespace ID.
 *
 * XXX: `id_ns' must be 4k aligned.
 */
static int
nvme_id_ns(struct nvme_state *s, struct nvme_id_ns *id_ns, uint16_t nsid)
{
    struct nvme_cmd cmd = {0};
    struct nvme_identify_cmd *identify = &cmd.identify;

    if (!is_4k_aligned(id_ns)) {
        return -1;
    }

    identify->opcode = NVME_OP_IDENTIFY;
    identify->nsid = nsid;
    identify->cns = 0;
    identify->prp1 = VIRT_TO_PHYS(id_ns);
    return nvme_poll_submit_cmd(&s->adminq, cmd);
}

/*
 * Init a namespace.
 *
 * @nsid: Namespace ID
 */
static int
nvme_init_ns(struct nvme_state *state, uint16_t nsid)
{
    char devname[128];
    struct nvme_ns *ns = NULL;
    struct nvme_id_ns *id_ns = NULL;
    struct device *dev;
    uint8_t lba_format;
    int status = 0;

    ns = dynalloc(sizeof(struct nvme_ns));
    if (ns == NULL) {
        status = -1;
        goto done;
    }

    id_ns = dynalloc_memalign(sizeof(struct nvme_id_ns), 0x1000);
    if ((status = nvme_id_ns(state, id_ns, nsid)) != 0) {
        dynfree(ns);
        goto done;
    }

    lba_format = id_ns->flbas & 0xF;
    ns->lba_fmt = id_ns->lbaf[lba_format];
    ns->nsid = nsid;
    ns->lba_bsize = 1 << ns->lba_fmt.ds;
    ns->size = id_ns->size;
    ns->cntl = state;
    nvme_create_ioq(ns, ns->nsid);

    dev = device_alloc();
    dev->read = nvme_dev_read;
    dev->write = nvme_dev_write;
    dev->blocksize = ns->lba_bsize;
    dev->mmap = NULL;
    ns->dev_id = device_create(dev, state->major, nsid);

    snprintf(devname, sizeof(devname), "nvme0n%d", nsid);
    if (devfs_add_dev(devname, dev) != 0) {
        KERR("Failed to create /dev/%s\n", devname);
    }

    TAILQ_INSERT_TAIL(&namespaces, ns, link);
done:
    if (id_ns != NULL)
        dynfree(id_ns);

    return status;
}

static int
nvme_disable_controller(struct nvme_state *state)
{
    struct nvme_bar *bar = state->bar;

    if (__TEST(bar->config, CONFIG_EN)) {
        bar->config &= ~CONFIG_EN;
    }

    if (nvme_poll_ready(bar, 0) < 0) {
        KERR("Failed to disable controller\n");
        return -1;
    }

    return 0;
}

/*
 * For debugging purposes, logs some information
 * found within the controller identify data structure.
 */
static void
nvme_log_ctrl_id(struct nvme_id *id)
{
    char mn[41] = {0};
    char fr[9] = {0};

    for (size_t i = 0; i < sizeof(id->mn); ++i) {
        mn[i] = id->mn[i];
    }
    for (size_t i = 0; i < sizeof(id->fr); ++i) {
        fr[i] = id->fr[i];
    }

    KDEBUG("NVMe model: %s\n", mn);
    KDEBUG("NVMe firmware revision: %s\n", fr);
}

/*
 * Fetch the list of namespace IDs
 *
 * @nsids_out: NSIDs will be written here via DMA.
 *
 * XXX: `nsids_out' must be 4k aligned.
 */
static int
nvme_get_nsids(struct nvme_state *state, uint32_t *nsids_out)
{
    struct nvme_cmd cmd = {0};
    struct nvme_identify_cmd *identify = &cmd.identify;

    if (!is_4k_aligned(nsids_out)) {
        return -1;
    }

    identify->opcode = NVME_OP_IDENTIFY;
    identify->cns = 2;      /* Active NSID list */
    identify->prp1 = VIRT_TO_PHYS(nsids_out);
    return nvme_poll_submit_cmd(&state->adminq, cmd);
}

static int
nvme_enable_controller(struct nvme_state *state)
{
    struct nvme_bar *bar = state->bar;
    struct nvme_id *id;

    uint32_t *nsids;
    uint8_t max_sqes, max_cqes;

    if (!__TEST(bar->config, CONFIG_EN)) {
        bar->config |= CONFIG_EN;
    }

    if (nvme_poll_ready(bar, 1) < 0) {
        KERR("Failed to enable controller\n");
        return -1;
    }

    id = dynalloc_memalign(sizeof(struct nvme_id), 0x1000);
    if (id == NULL) {
        return -1;
    }

    nsids = dynalloc_memalign(0x1000, 0x1000);
    if (nsids == NULL) {
        return -1;
    }

    nvme_identify(state, id);
    nvme_log_ctrl_id(id);
    nvme_get_nsids(state, nsids);

    /*
     * Before creating any I/O queues we need to set CC.IOCQES
     * and CC.IOSQES... Bits 3:0 is the minimum and bits 7:4
     * is the maximum. We'll choose the maximum.
     */
    max_sqes = id->sqes >> 4;
    max_cqes = id->cqes >> 4;
    bar->config |= (max_sqes << CONFIG_IOSQES_SHIFT);
    bar->config |= (max_cqes << CONFIG_IOCQES_SHIFT);

    /* Init NVMe namespaces */
    for (size_t i = 0; i < id->nn; ++i) {
        if (nsids[i] != 0) {
            KINFO("Found NVMe namespace (id=%d)\n", nsids[i]);
            nvme_init_ns(state, nsids[i]);
        }
    }

    dynfree(nsids);
    dynfree(id);
    return 0;
}

static int
nvme_init_controller(struct nvme_bar *bar)
{
    struct nvme_state state = { . bar = bar };
    struct nvme_queue *adminq = &state.adminq;

    uint16_t mqes = CAP_MQES(bar->caps);
    uint16_t cmdreg_bits = PCI_BUS_MASTERING |
                           PCI_MEM_SPACE;

    pci_set_cmdreg(nvme_dev, cmdreg_bits);
    nvme_disable_controller(&state);

    nvme_create_queue(&state, adminq, 0);

    /* Setup admin submission and admin completion queues */
    bar->aqa = (mqes | mqes << 16);
    bar->asq = VIRT_TO_PHYS(adminq->sq);
    bar->acq = VIRT_TO_PHYS(adminq->cq);

    state.major = device_alloc_major();
    return nvme_enable_controller(&state);
}

static int
nvme_init(void)
{
    struct nvme_bar *bar;
    struct pci_lookup nvme_lookup = {
        .pci_class = 1,
        .pci_subclass = 8
    };

    if (req_timer(TIMER_GP, &driver_tmr) != 0) {
        KERR("Failed to fetch general purpose timer\n");
        return -1;
    }

    if (driver_tmr.msleep == NULL) {
        KERR("Timer does not have msleep()\n");
        return -1;
    }

    nvme_dev = pci_get_device(nvme_lookup, PCI_CLASS | PCI_SUBCLASS);
    if (nvme_dev == NULL) {
        return -1;
    }

    bar = (struct nvme_bar *)(nvme_dev->bar[0] & ~7);
    KINFO("NVMe BAR0 @ 0x%p\n", bar);
    TAILQ_INIT(&namespaces);

    if (nvme_init_controller(bar) < 0) {
        return -1;
    }

    return 0;
}

DRIVER_EXPORT(nvme_init);

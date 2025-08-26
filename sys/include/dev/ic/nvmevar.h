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

#ifndef _IC_NVMEVAR_H_
#define _IC_NVMEVAR_H_

#include <sys/types.h>
#include <sys/cdefs.h>

/* Admin commands */
#define NVME_OP_CREATE_IOSQ     0x01
#define NVME_OP_GET_LOGPAGE     0x02
#define NVME_OP_CREATE_IOCQ     0x05
#define NVME_OP_IDENTIFY        0x06

/* Identify CNS values */
#define ID_CNS_CTRL         0x01    /* Identify controller */
#define ID_CNS_NSID_LIST    0x02    /* Active NSID list */

/* I/O commands */
#define NVME_OP_WRITE 0x01
#define NVME_OP_READ 0x02

/* Log page identifiers */
#define NVME_LOGPAGE_SMART 0x02

/*
 * S.M.A.R.T health / information log
 *
 * See section 5.16.1.3, figure 207 of the
 * NVMe base spec (rev 2.0a)
 *
 * @cwarn: Critical warning
 * @temp: Composite tempature (kelvin)
 * @avail_spare: Available spare (in percentage)
 * @avail_spare_thr: Available spare threshold
 * @percent_used: Estimate NVMe life used percentage
 * @end_cwarn: Endurance group critical warning summary
 * @data_units_read: Number of 512 byte data units read
 * @data_units_written: Number of 512 byte data units written
 * @host_reads: Number of host read commands completed
 * @host_writes: Number of host write commands completed
 * @ctrl_busy_time: Controller busy time
 * @power_cycles: Number of power cycles
 * @power_on_hours: Number of power on hours
 * @unsafe_shutdowns: Number of unsafe shutdowns
 * @media_errors: Media and data integrity errors
 * @n_errlog_entries: Number of error log info entries
 * @warning_temp_time: Warning composite tempature time
 * @critical_comp_time: Critical composite tempature time
 * @temp_sensor: Tempature sensor <n> data
 * @temp1_trans_cnt: Tempature 1 transition count
 * @temp2_trans_cnt: Tempature 2 transition count
 * @temp1_total_time: Total time for tempature 1
 * @temp2_total_time: Total time for tempature 2
 */
struct __packed nvme_smart_data {
    uint8_t cwarn;
    uint16_t temp;
    uint8_t avail_spare;
    uint8_t avail_spare_thr;
    uint8_t percent_used;
    uint8_t end_cwarn;
    uint8_t reserved[25];
    uint8_t data_units_read[16];
    uint8_t data_units_written[16];
    uint8_t host_reads[16];
    uint8_t host_writes[16];
    uint8_t ctrl_busy_time[16];
    uint8_t power_cycles[16];
    uint8_t power_on_hours[16];
    uint8_t unsafe_shutdowns[16];
    uint8_t media_errors[16];
    uint8_t n_errlog_entries[16];
    uint32_t warning_temp_time;
    uint32_t critical_comp_time;
    uint16_t temp_sensor[8];
    uint32_t temp1_trans_cnt;
    uint32_t temp2_trans_cnt;
    uint32_t temp1_total_time;
    uint32_t temp2_total_time;
    uint8_t reserved1[280];
};

struct nvme_identify_cmd {
    uint8_t opcode;
    uint8_t flags;
    uint16_t cid;
    uint32_t nsid;
    uint64_t unused1;
    uint64_t unused2;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cns;
    uint32_t unused3[5];
};

/* Command completion queue entry */
struct nvme_cq_entry {
    uint32_t res;
    uint32_t unused;
    uint16_t sqhead;
    uint16_t sqid;
    uint16_t cid;
    uint16_t status;
};

/* Create I/O completion queue */
struct nvme_create_iocq_cmd {
    uint8_t opcode;
    uint8_t flags;
    uint16_t cid;
    uint32_t unused1[5];
    uint64_t prp1;
    uint64_t unused2;
    uint16_t qid;
    uint16_t qsize;
    uint16_t qflags;
    uint16_t irqvec;
    uint64_t unused3[2];
};

/* Create I/O submission queue */
struct nvme_create_iosq_cmd {
    uint8_t opcode;
    uint8_t flags;
    uint16_t cid;
    uint32_t unused1[5];
    uint64_t prp1;
    uint64_t unused2;
    uint16_t sqid;
    uint16_t qsize;
    uint16_t qflags;
    uint16_t cqid;
    uint64_t unused3[2];
};

/* Get log page */
struct nvme_get_logpage_cmd {
    uint8_t opcode;
    uint8_t flags;
    uint16_t cid;
    uint32_t nsid;
    uint64_t unused[2];
    uint64_t prp1;
    uint64_t prp2;
    uint8_t lid;
    uint8_t lsp;
    uint16_t numdl;
    uint16_t numdu;
    uint16_t lsi;
    uint64_t lpo;
    uint8_t unused1[3];
    uint8_t csi;
    uint32_t unused2;
};

/* Read/write */
struct nvme_rw_cmd {
    uint8_t opcode;
    uint8_t flags;
    uint16_t cid;
    uint32_t nsid;
    uint64_t unused;
    uint64_t metadata;
    uint64_t prp1;
    uint64_t prp2;
    uint64_t slba;
    uint16_t len;
    uint16_t control;
    uint32_t dsmgmt;
    uint32_t ref;
    uint16_t apptag;
    uint16_t appmask;
};

struct nvme_cmd {
    union {
        struct nvme_identify_cmd identify;
        struct nvme_create_iocq_cmd create_iocq;
        struct nvme_create_iosq_cmd create_iosq;
        struct nvme_get_logpage_cmd get_logpage;
        struct nvme_rw_cmd rw;
    };
};

struct nvme_queue {
    struct nvme_cmd *sq;        /* Submission queue */
    struct nvme_cq_entry *cq;   /* Completion queue */
    uint16_t sq_head;           /* Submission queue head */
    uint16_t sq_tail;           /* Submission queue tail */
    uint16_t cq_head;           /* Completion queue head */
    uint8_t cq_phase : 1;       /* Completion queue phase bit */
    uint16_t size;              /* Size in elements */
    volatile uint32_t *sq_db;   /* Submission doorbell */
    volatile uint32_t *cq_db;   /* Completion doorbell */
};

struct nvme_id {
    uint16_t vid;
    uint16_t ssvid;
    char sn[20];
    char mn[40];
    char fr[8];
    uint8_t rab;
    uint8_t ieee[3];
    uint8_t mic;
    uint8_t mdts;
    uint16_t ctrlid;
    uint32_t version;
    uint32_t unused1[43];
    uint16_t oacs;
    uint8_t acl;
    uint8_t aerl;
    uint8_t fw;
    uint8_t lpa;
    uint8_t elpe;
    uint8_t npss;
    uint8_t avscc;
    uint8_t apsta;
    uint16_t wctemp;
    uint16_t cctemp;
    uint16_t unused2[121];
    uint8_t sqes;
    uint8_t cqes;
    uint16_t unused3;
    uint32_t nn;
    uint16_t oncs;
    uint16_t fuses;
    uint8_t fna;
    uint8_t vwc;
    uint16_t awun;
    uint16_t awupf;
    uint8_t nvscc;
    uint8_t unused4;
    uint16_t acwu;
    uint16_t unused5;
    uint32_t sgls;
    uint32_t unused6[1401];
    uint8_t vs[1024];
};

struct nvme_lbaf {
    uint16_t ms;    /* Number of metadata bytes per LBA */
    uint8_t ds;     /* Data size */
    uint8_t rp;
};

/* Identify namespace data */
struct nvme_id_ns {
    uint64_t size;
    uint64_t capabilities;
    uint64_t nuse;
    uint8_t features;
    uint8_t nlbaf;
    uint8_t flbas;
    uint8_t mc;
    uint8_t dpc;
    uint8_t dps;
    uint8_t nmic;
    uint8_t rescap;
    uint8_t fpi;
    uint8_t unused1;
    uint16_t nawun;
    uint16_t nawupf;
    uint16_t nacwu;
    uint16_t nabsn;
    uint16_t nabo;
    uint16_t nabspf;
    uint16_t unused2;
    uint64_t nvmcap[2];
    uint64_t unusued3[5];
    uint8_t nguid[16];
    uint8_t eui64[8];
    struct nvme_lbaf lbaf[16];
    uint64_t unused3[24];
    uint8_t vs[3712];
};

/* NVMe namespace */
struct nvme_ns {
    size_t nsid;                /* Namespace ID */
    size_t lba_bsize;           /* LBA block size */
    size_t size;                /* Size in logical blocks */
    struct nvme_queue ioq;      /* I/O queue */
    struct nvme_lbaf lba_fmt;   /* LBA format */
    struct nvme_ctrl *ctrl;     /* NVMe controller */
    dev_t dev;
    TAILQ_ENTRY(nvme_ns) link;
};

struct nvme_ctrl {
    struct nvme_queue adminq;
    struct nvme_bar *bar;
    uint8_t sqes;
    uint8_t cqes;
};

#endif  /* !_IC_NVMEVAR_H_ */

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

#ifndef _IC_NVMEVAR_H_
#define _IC_NVMEVAR_H_

#include <sys/types.h>

/* Admin commands */
#define NVME_OP_CREATE_IOSQ     0x01
#define NVME_OP_CREATE_IOCQ     0x05
#define NVME_OP_IDENTIFY        0x06

/* Identify CNS values */
#define ID_CNS_CTRL         0x01    /* Identify controller */
#define ID_CNS_NSID_LIST    0x07    /* Active NSID list */

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

struct nvme_cmd {
    union {
        struct nvme_identify_cmd identify;
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

struct nvme_ctrl {
    struct nvme_queue adminq;
};

#endif  /* !_IC_NVMEVAR_H_ */

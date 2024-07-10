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

#ifndef _IC_NVMEREGS_H_
#define _IC_NVMEREGS_H_

#include <sys/types.h>
#include <sys/param.h>

/* Controller Capabilities */
#define CAP_MPSMIN(caps) ((caps >> 48) & 0xF)
#define CAP_MPSMAX(caps) ((caps >> 52) & 0xF)
#define CAP_TIMEOUT(caps) ((caps >> 24) & 0xFF)
#define CAP_STRIDE(caps) ((caps >> 32) & 0xF)
#define CAP_MQES(caps) (caps & 0xFFFF)
#define CAP_CSS(caps) (caps & 0xFF)

/* Controller Configuration */
#define CONFIG_EN           BIT(0)
#define CONFIG_CSS_SHIFT    4
#define CONFIG_IOSQES_SHIFT 16
#define CONFIG_IOCQES_SHIFT 20

/* Controller status */
#define STATUS_RDY  BIT(0)
#define STATUS_CFS  BIT(1)

/* Command sets supported */
#define CSS_NVM     BIT(0)
#define CSS_IO      BIT(6)
#define CSS_NO_IO   BIT(7)

/* NVMe controller */
struct nvme_bar {
    volatile uint64_t caps;
    volatile uint32_t version;
    volatile uint32_t intms;     /* Interrupt mask set */
    volatile uint32_t intmc;     /* Interrupt mask clear */
    volatile uint32_t config;
    volatile uint32_t unused1;
    volatile uint32_t status;
    volatile uint32_t unused2;
    volatile uint32_t aqa;       /* Admin queue attributes */
    volatile uint64_t asq;       /* Admin submission queue */
    volatile uint64_t acq;       /* Admin completion queue */
};

#endif  /* !_IC_NVMEREGS_H_ */

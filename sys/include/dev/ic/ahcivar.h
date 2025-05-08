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

#ifndef _IC_AHCIVAR_H_
#define _IC_AHCIVAR_H_

#include <sys/param.h>
#include <sys/types.h>
#include <dev/ic/ahciregs.h>

/*
 * AHCI Host Bus Adapter
 *
 * @io: HBA MMIO
 * @maxports: Max number of HBA ports
 * @nports: Number of implemented HBA ports.
 * @nslots: Number of command slots
 * @ems: Enclosure management support
 * @sal: Supports activity LED
 */
struct ahci_hba {
    struct hba_memspace *io;
    uint32_t maxports;
    uint32_t nports;
    uint32_t nslots;
    uint8_t ems  : 1;
    uint8_t sal  : 1;
};

/*
 * A device attached to a physical HBA port.
 *
 * @io: Memory mapped port registers
 * @hba: HBA descriptor
 * @dev: Device minor number.
 */
struct hba_device {
    struct hba_port *io;
    struct ahci_hba *hba;
    dev_t dev;
};

/*
 * Command header
 *
 * @cfl: Command FIS length
 * @a: ATAPI
 * @w: Write
 * @p: Prefetchable
 * @r: Reset
 * @c: Clear busy upon R_OK
 * @rsvd0: Reserved
 * @pmp: Port multiplier port
 * @prdtl: PRDT length (in entries)
 * @prdbc: PRDT bytes transferred count
 * @ctba: Command table descriptor base addr
 * @rsvd1: Reserved
 */
struct ahci_cmd_hdr {
    uint8_t cfl : 5;
    uint8_t a   : 1;
    uint8_t w   : 1;
    uint8_t p   : 1;
    uint8_t r   : 1;
    uint8_t c   : 1;
    uint8_t rsvd0 : 1;
    uint8_t pmp   : 4;
    uint16_t prdtl;
    volatile uint32_t prdbc;
    uintptr_t ctba;
    uint32_t rsvd1[4];
};

/*
 * Host to device FIS
 *
 * [h]: Set by host
 * [d]: Set by device
 * [srb]: Shadow register block
 *
 * @type: Must be 0x27 for H2D [h]
 * @pmp: Port multiplier port [h]
 * @c: Set to denote command FIS [h]
 * @command: Command type [h/srb]
 * @feature1: Features register (7:0) [h/srb]
 * @lba0: LBA low [h/srb]
 * @lba1: LBA mid [h/srb]
 * @lba2: LBA hi  [h/srb]
 * @device: Set bit 7 for LBA [h/srb]
 * @lba3: LBA (31:24) [h/srb]
 * @lba4: LBA (39:32) [h/srb]
 * @lba5: LBA (47:40) [h/srb]
 * @featureh: Features high [h/srb]
 * @countl: Count low (block aligned) [h/srb]
 * @counth: Count high (block aligned) [h/srb]
 */
struct ahci_fis_h2d {
    uint8_t type;
    uint8_t pmp : 4;
    uint8_t rsvd0 : 3;
    uint8_t c : 1;
    uint8_t command;
    uint8_t featurel;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    uint8_t rsvd1[4];
};

#define AHCI_TIMEOUT 500    /* In ms */

#endif  /* !_IC_AHCIVAR_H_ */

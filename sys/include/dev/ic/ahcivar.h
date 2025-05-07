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

#define AHCI_TIMEOUT 500    /* In ms */

#endif  /* !_IC_AHCIVAR_H_ */

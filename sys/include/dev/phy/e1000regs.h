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

#ifndef _PHY_E1000_REGS_H_
#define _PHY_E1000_REGS_H_

#include <sys/types.h>
#include <sys/param.h>

/*
 * Device I/O memory space
 *
 * @ctl: Control register
 * @status: Status register
 * @eecd: EEPROM/flash control & data register
 * @eerd: EEPROM/flash read register (see reserved notes below)
 * @fla: Flash access register (see reserved notes below)
 * @ctl_ext: Extended device control register
 * @midi_ctl: PHY management data interface control register
 * @fctl: Flow control register
 * @fct: Flow control type register
 * @vet: VLAN ether type register
 * @fcttv: Flow control transmit timer value register
 * @txcw: Transmit config word register (see reserved notes below)
 * @rxcw: Receive config word register (see reserved notes below)
 * @ledctl: LED control register (see reserved notes below)
 *
 * XXX: Notes about reserve fields:
 *
 *      - The `eerd' register is reserved and should NOT be touched
 *        for the 82544GC/EI card.
 *
 *      - The `fla' register is only usable for the 82541xx and
 *        82547GI/EI cards, this is reserved and should NOT be
 *        touched on any other cards.
 *
 *      - The `txcw' and `rxcw' registers are reserved and should NOT
 *        be touched for the 82540EP/EM, 82541xx and 82547GI/EI cards.
 *
 *      - The `ledctl' register is reserved and should NOT be touched
 *        for the 82544GC/EI card.
 */
struct e1000_iomem {
    volatile uint32_t ctl;
    volatile uint32_t status;
    volatile uint32_t eecd;
    volatile uint32_t eerd;
    volatile uint32_t fla;
    volatile uint32_t ctl_ext;
    volatile uint32_t midi_ctl;
    volatile uint64_t fctl;
    volatile uint32_t fct;
    volatile uint32_t vet;
    volatile uint32_t fcttv;
    volatile uint32_t txcw;
    volatile uint32_t rxcw;
    volatile uint32_t ledctl;
};

/*
 * Device control register (`ctl') bits
 *
 * See section 13.4.1 of the PCI/PCI-X Intel Gigabit
 * Ethernet Controllers spec
 *
 * XXX: Notes about reserved bits:
 *
 *      - The CTL.LRST bit is reserved and should NOT be touched
 *        for the 82540EP/EM, 82541xx, or 82547GI/EI cards.
 */
#define E1000_CTL_FD    BIT(0)     /* Full-duplex */
#define E1000_CTL_LRST  BIT(3)     /* Link-reset */
#define E1000_CTL_RST   BIT(26)    /* Device reset */

/*
 * EEPROM/flash control and data register (`eecd')
 * bits
 *
 * See section 13.4.3 of the PCI/PCI-X Intel Gigabit
 * Ethernet controller spec
 */
#define E1000_EECD_SK   BIT(0)  /* EEPROM clock input */
#define E1000_EECD_CS   BIT(1)  /* EEPROM chip select */
#define E1000_EECD_DI   BIT(2)  /* EEPROM data input */
#define E1000_EECD_DO   BIT(3)  /* EEPROM data output */
#define E1000_EECD_FWE  BIT(4)  /* EEPROM flash write enable ctl (4:5) */
#define E1000_EECD_REQ  BIT(6)  /* Request EEPROM access */
#define E1000_EECD_GNT  BIT(7)  /* Grant EEPROM access */
#define E1000_EECD_PRES BIT(8)  /* EEPROM present */
#define E1000_EECD_SIZE BIT(9)  /* EEPROM size (1024-bit [0], 4096-bit [1]) */
#define E1000_EECD_TYPE BIT(13) /* EEPROM type (microwire [0], SPI [1]) */

#endif  /* !_PHY_E1000_REGS_H_ */

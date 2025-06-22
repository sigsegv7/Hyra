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
 * E1000 register offsets
 *
 * XXX: Notes about reserve fields:
 *
 *  - The `EERD' register is reserved and should NOT be touched
 *    for the 82544GC/EI card.
 *
 *  - The `FLA' register is only usable for the 82541xx and
 *    82547GI/EI cards, this is reserved and should NOT be
 *    touched on any other cards.
 *
 *  - The `TXCW' and `RXCW' registers are reserved and should NOT
 *    be touched for the 82540EP/EM, 82541xx and 82547GI/EI cards.
 *
 *  - The `LEDCTL' register is reserved and should NOT be touched
 *    for the 82544GC/EI card.
 */
#define E1000_CTL       0x00000  /* Control register */
#define E1000_STATUS    0x00008  /* Status register */
#define E1000_EECD      0x00010  /* EEPROM/flash control and data register */
#define E1000_EERD      0x00014  /* EEPROM/flash read register */
#define E1000_FLA       0x0001C  /* EEPROM/flash read register */
#define E1000_CTRL_EXT  0x00018  /* Extended device control register */
#define E1000_MDIC      0x00020  /* PHY management data interface control register */
#define E1000_FCAL      0x00028  /* Flow control low register */
#define E1000_FCAH      0x0002C  /* Flow control high register */
#define E1000_FCT       0x00030  /* Flow control type register */
#define E1000_VET       0x00038  /* VLAN ethertype register */
#define E1000_FCTTV     0x00170  /* Flow control transmit timer value register */
#define E1000_TXCW      0x00178  /* Transmit config word register */
#define E1000_RXCW      0x00180  /* Receive config word register */
#define E1000_LEDCTL    0x00E00  /* LED control register */

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

/*
 * EEPROM read (`eerd') register bits
 *
 * See section 13.4.4 of the PCI/PCI-X Intel Gigabit
 * Ethernet controller spec
 */
#define E1000_EERD_START     BIT(0) /* Start read */
#define E1000_EERD_DONE      BIT(4) /* EEPROM read finished */

/*
 * EEPROM word addresses
 */
#define E1000_HWADDR0 0x00      /* Word 0 */
#define E1000_HWADDR1 0x01      /* Word 1 */
#define E1000_HWADDR2 0x02      /* Word 2 */

#endif  /* !_PHY_E1000_REGS_H_ */

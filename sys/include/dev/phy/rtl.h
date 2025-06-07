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

#ifndef _PHY_RT8139_H_
#define _PHY_RT8139_H_

#include <sys/types.h>
#include <sys/param.h>

/* MAC address */
#define RT_IDR0         0x00
#define RT_IDR1         0x00
#define RT_IDR2         0x02
#define RT_IDR3         0x03
#define RT_IDR4         0x04
#define RT_IDR5         0x05

#define RT_MAR0         0x08    /* Multicast filter */
#define RT_TXSTATUS0    0x10    /* Transmit status (4 32bit regs) */
#define RT_TXADDR0      0x20    /* Tx descriptors (also 4 32bit) */
#define RT_RXBUF        0x30    /* Receive buffer start address */
#define RT_RXEARLYCNT   0x34    /* Early Rx byte count */
#define RT_RXEARLYSTATUS 0x36   /* Early Rx status */
#define RT_CHIPCMD      0x37    /* Command register */
#define RT_RXBUFTAIL    0x38    /* Current address of packet read (queue tail) */
#define RT_RXBUFHEAD    0x3A    /* Current buffer address (queue head) */
#define RT_INTRMASK     0x3C    /* Interrupt mask */
#define RT_INTRSTATUS   0x3E    /* Interrupt status */
#define RT_TXCONFIG     0x40    /* Tx config */
#define RT_RXCONFIG     0x44    /* Rx config */
#define RT_TIMER        0x48    /* A general purpose counter */
#define RT_RXMISSED     0x4C    /* 24 bits valid, write clears */
#define RT_CFG9346      0x50    /* 93C46 command register */
#define RT_CONFIG0      0x51    /* Configuration reg 0 */
#define RT_CONFIG1      0x52    /* Configuration reg 1 */
#define RT_TIMERINT     0x54    /* Timer interrupt register (32 bits) */
#define RT_MEDIASTATUS  0x58    /* Media status register */
#define RT_CONFIG3      0x59    /* Config register 3 */
#define RT_CONFIG4      0x5A    /* Config register 4 */
#define RT_MULTIINTR    0x5C    /* Multiple interrupt select */
#define RT_MII_TSAD     0x60    /* Transmit status of all descriptors (16 bits) */
#define RT_MII_BMCR     0x62    /* Basic Mode Control Register (16 bits) */
#define RT_MII_BMSR     0x64    /* Basic Mode Status Register (16 bits) */
#define RT_AS_ADVERT    0x66    /* Auto-negotiation advertisement reg (16 bits) */
#define RT_AS_LPAR      0x68    /* Auto-negotiation link partner reg (16 bits) */
#define RT_AS_EXPANSION 0x6A    /* Auto-negotiation expansion reg (16 bits) */

/* Command register bits */
#define RT_BUFEN BIT(0)         /* Buffer empty */
#define RT_TE    BIT(2)         /* Transmitter enable */
#define RT_RE    BIT(3)         /* Receiver enable */
#define RT_RST   BIT(4)         /* Reset */

/* 93C46 EEPROM mode bits */
#define RT_EEM0  BIT(6)
#define RT_EEM1  BIT(7)

/* Receive register bits */
#define RT_AAP   BIT(0)         /* Accept all packets */
#define RT_APM   BIT(1)         /* Accept physical match packets */
#define RT_AM    BIT(2)         /* Accept multicast packets */
#define RT_AB    BIT(3)         /* Accept brodcast packets */
#define RT_AR    BIT(4)         /* Accept runt */
#define RT_AER   BIT(5)         /* Accept error packet */
#define RT_WRAP  BIT(6)         /* RX wrap */

/* Interrupt mask bits */
#define RT_ROK  BIT(0)          /* Receive OK */
#define RT_RER  BIT(1)          /* Receive error */
#define RT_TOK  BIT(2)          /* Transmit OK */
#define RT_ACKW 0x0005          /* RX/TX ACK word */

/* Register poll timeout */
#define RT_TIMEOUT_MSEC 500

#endif  /* !_PHY_RT8139_H_ */

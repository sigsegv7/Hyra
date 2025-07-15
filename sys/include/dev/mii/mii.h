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

#ifndef _DEV_MII_H_
#define _DEV_MII_H_

#include <sys/param.h>

/*
 * MII registers
 */
#define MII_BMCR    0x00    /* Basic Mode Config */
#define MII_BMSR    0x01    /* Basic Mode Status */
#define MII_PHYID   0x02    /* MII PHY identifier 1 */
#define MII_PHYID2  0x03    /* MII PHY identifier 2 */
#define MII_ADVER   0x04    /* Auto-negotiation advertisement */
#define MII_LPA     0x05    /* Link parter abilities */
#define MII_EXPAN   0x06    /* Auto-negotiation expansion */
#define MII_ESTATUS 0x0F    /* Extended status register */
#define MII_IRQ     0x1B    /* Interrupt control/status */

/*
 * MII BMCR bits
 */
#define MII_BMCR_RST  BIT(15)   /* PHY reset */
#define MII_BCMR_LOOP BIT(14)   /* Loopback mode enable */
#define MII_BMCR_ANEN BIT(12)   /* Auto-negotiation enable */
#define MII_PWR_DOWN  BIT(11)   /* Power down PHY */
#define MII_ISOLATE   BIT(10)   /* Electrically isolate PHY from MII */

#endif  /* !_DEV_MII_H_ */

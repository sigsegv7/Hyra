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

#ifndef _I8042VAR_H_
#define _I8042VAR_H_

#include <sys/param.h>

/* I/O bus ports */
#define I8042_CMD       0x64
#define I8042_DATA      0x60
#define I8042_STATUS    0x64

#define KB_IRQ      1       /* Keyboard IRQ */
#define I8042_DELAY 20      /* Poll delay (in ms) */

/* i8042 status */
#define I8042_OBUFF BIT(0)  /* Output buffer full */
#define I8042_IBUFF BIT(1)  /* Input buffer full */
#define I8042_TTO   BIT(5)  /* Transmit timeout */
#define I8042_RTO   BIT(6)  /* Receive timeout */
#define I8042_PAR   BIT(7)  /* Parity error */

/* i8042 commands */
#define I8042_SELF_TEST     0xAA
#define I8042_DISABLE_PORT0 0xAD
#define I8042_DISABLE_PORT1 0xA7
#define I8042_ENABLE_PORT0  0xAE
#define I8042_ENABLE_PORT1  0xA8
#define I8042_GET_CONFB     0x20
#define I8042_SET_CONFB     0x60
#define I8042_PORT1_SEND    0xD4

/* i8042 config bits */
#define I8042_PORT0_INTR    BIT(0)
#define I8042_PORT1_INTR    BIT(1)
#define I8042_PORT0_CLK     BIT(4)
#define I8042_PORT1_CLK     BIT(5)

/* Aux commands */
#define I8042_AUX_DEFAULTS 0xF5
#define I8042_AUX_ENABLE   0xF4
#define I8042_AUX_DISABLE  0xF5
#define I8042_AUX_RESET    0xFF

/* LED bits */
#define I8042_LED_SCROLL    BIT(0)
#define I8042_LED_NUM       BIT(1)
#define I8042_LED_CAPS      BIT(2)

/* Quirks */
#define I8042_HOSTILE       BIT(0)      /* If EC likes throwing NMIs */

/* Apply a quirk to i8042 */
void i8042_quirk(int mask);

/* Internal - do not use */
void i8042_sync(void);
void i8042_kb_isr(void);
void i8042_kb_event(void);

#endif  /* _I8042VAR_H_ */

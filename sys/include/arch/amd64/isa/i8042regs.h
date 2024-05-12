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

#ifndef _AMD64_I8042REG_H_
#define _AMD64_I8042REG_H_

#include <sys/cdefs.h>

#define I8042_DATA      0x60
#define I8042_STATUS    0x64
#define I8042_COMMAND   0x64

/* Status register bits */
#define I8042_OBUF_FULL     __BIT(0)    /* Output buffer full */
#define I8042_IBUF_FULL     __BIT(1)    /* Input buffer full */

/* Commands */
#define I8042_DISABLE_PORT0 0xAD
#define I8042_DISABLE_PORT1 0xA7
#define I8042_ENABLE_PORT0  0xAE
#define I8042_ENABLE_PORT1  0xA8
#define I8042_GET_CONFB     0x20
#define I8042_SET_CONFB     0x60
#define I8042_PORT1_SEND    0xD4

/* Controller config bits */
#define I8042_PORT0_INTR    __BIT(0)
#define I8042_PORT1_INTR    __BIT(1)
#define I8042_PORT0_CLK     __BIT(4)
#define I8042_PORT1_CLK     __BIT(5)

/* Aux commands */
#define I8042_AUX_DEFAULTS 0xF5
#define I8042_AUX_ENABLE   0xF4
#define I8042_AUX_DISABLE  0xF5
#define I8042_AUX_RESET    0xFF

/* LED bits */
#define I8042_LED_SCROLL    __BIT(0)
#define I8042_LED_NUM       __BIT(1)
#define I8042_LED_CAPS      __BIT(2)

#endif  /* !_AMD64_I8042REG_H_ */

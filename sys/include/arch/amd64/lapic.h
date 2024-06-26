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

#ifndef _AMD64_LAPIC_H_
#define _AMD64_LAPIC_H_

#include <sys/types.h>

#define LAPIC_TMR_ONESHOT   0x00
#define LAPIC_TMR_PERIODIC  0x01

/* IPI Destination Shorthands */
enum {
    IPI_SHORTHAND_NONE,
    IPI_SHORTHAND_SELF,
    IPI_SHORTHAND_ALL,
    IPI_SHORTHAND_OTHERS
};

/* IPI Destination Modes */
enum {
    IPI_DEST_PHYSICAL,
    IPI_DEST_LOGICAL
};

void lapic_timer_init(size_t *freq_out);
void lapic_timer_oneshot(bool mask, uint32_t count);
void lapic_timer_oneshot_us(size_t us);
void lapic_send_ipi(uint8_t id, uint8_t shorthand, uint8_t vector);
void lapic_send_eoi(void);
void lapic_init(void);

#endif  /* !_AMD64_LAPIC_H_ */

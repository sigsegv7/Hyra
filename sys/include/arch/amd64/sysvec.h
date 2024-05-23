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

#ifndef _AMD64_SYSVEC_H_
#define _AMD64_SYSVEC_H_

#include <sys/cdefs.h>

/*
 * Interrupt vectors reserved
 * to core system usage excluding
 * device IRQs routed by 8259 PIC,
 * I/O APIC, etc.
 */
typedef enum {
    SYSVEC_LAPIC_TIMER = 0x21,     /* Local APIC timer */
    SYSVEC_IPI,                    /* IPI vector */
    SYSVEC_HLT,                    /* Halt vector */
    SYSVEC_PCKBD,                  /* Keyboard vector */
    SYSVEC_TLB,                    /* TLB shootdown */

    /* -- XXX: New vectors go above -- */
    NSYSVEC_BASE,                  /* Non-system vector base */
} sysvec_t;

/* Unlikely, but just in case */
__STATIC_ASSERT(NSYSVEC_BASE <= 0xFF, "VECTOR OVERFLOW CAUGHT");

#endif  /* !_AMD64_SYSVEC_H_ */

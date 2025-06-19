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

#ifndef _MACHINE_EXCEPTION_H_
#define _MACHINE_EXCEPTION_H_

#include <sys/types.h>
#include <machine/frame.h>

/* Exception class */
#define EC_UNKNOWN  0x00    /* Unknown type */
#define EC_WF       0x01    /* Trapped WF instruction */
#define EC_MCRMRC   0x03    /* Trapped MCR/MRC */
#define EC_MCRRC    0x04    /* Trapped MCRR/MRRC */
#define EC_LDCSTC   0x06    /* Trapped LDC/STC */
#define EC_SVE      0x07    /* Trapped SVE/SIMD/FP op */
#define EC_BRE      0x0D    /* Branch target exception */
#define EC_ILLX     0x0E    /* Illegal execution state */
#define EC_SVC64    0x15    /* AARCH64 SVC */
#define EC_PCALIGN  0x22    /* PC alignment fault */
#define EC_DABORT   0x24    /* Data abort (w/o ELx change) */
#define EC_EDABORT  0x25    /* Data abort (w/ ELx change) */
#define EC_SPALIGN  0x26    /* SP alignment fault */
#define EC_SERR     0x2F    /* System error (what the fuck!) */

void handle_exception(struct trapframe *tf);

#endif  /* !_MACHINE_EXCEPTION_H_ */

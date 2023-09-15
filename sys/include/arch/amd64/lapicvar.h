/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

#ifndef _AMD64_LAPICVAR_H_
#define _AMD64_LAPICVAR_H_

/* LAPIC register offsets */
#define LAPIC_ID            0x0020U     /* ID Register */
#define LAPIC_VERSION       0x0030U     /* Version Register */
#define LAPIC_TPR           0x0080U     /* Task Priority Register */
#define LAPIC_APR           0x0090U     /* Arbitration Priority Register */
#define LAPIC_PPR           0x00A0U     /* Processor Priority Register */
#define LAPIC_EOI           0x00B0U     /* End Of Interrupt Register */
#define LAPIC_RRD           0x00C0U     /* Remote Read Register */
#define LAPIC_LDR           0x00D0U     /* Logical Destination Register */
#define LAPIC_DFR           0x00E0U     /* Destination Format Register */
#define LAPIC_SVR           0x00F0U     /* Spurious Vector Register */
#define LAPIC_ISR           0x0100U     /* In service register (max=0x0220) */
#define LAPIC_TMR           0x0180U     /* Trigger Mode Register (max=0x0220) */
#define LAPIC_IRR           0x0200U     /* Interrupt Request Register (max=0x0270) */
#define LAPIC_ERR           0x0280U     /* Error Status Register */
#define LAPIC_DCR           0x03E0U     /* Divide Configuration Register (for timer) */

#endif  /* _AMD64_LAPICVAR_H_ */

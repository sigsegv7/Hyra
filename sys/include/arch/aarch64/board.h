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

#ifndef _ARCH_AARCH64_BOARD_H_
#define _ARCH_AARCH64_BOARD_H_

#include <sys/cdefs.h>

#if defined(__aarch64__)

static inline const char *
aarch64_get_board(void)
{
    uint32_t midr_el1;
    __asm("mrs %x0, midr_el1"
          : "=r" (midr_el1)
    );

    switch ((midr_el1 >> 4) & 0xFFF) {
    case 0xB76:           /* MMIO: 0x20000000 */
        return "Raspberry Pi 1";
        break;
    case 0xC07:           /* MMIO: 0x3F000000 */
        return "Raspberry Pi 2";
        break;
    case 0xD03:           /* MMIO: 0x3F000000 */
        return "Raspberry Pi 3";
        break;
    case 0xD08:            /* MMIO: 0xFE000000 */
        return "Raspberry Pi 4";
        break;
    default:               /* MMIO: 0x20000000 */
        return "Unknown";
        break;
    }
}

static inline uintptr_t
aarch64_get_mmio_base(void)
{
    uint32_t midr_el1;
    __asm("mrs %x0, midr_el1"
          : "=r" (midr_el1)
    );

    switch ((midr_el1 >> 4) & 0xFFF) {
    case 0xB76:
        return 0x20000000;
    case 0xC07:
    case 0xCD03:
        return 0x3F000000;
    case 0xD08:
        return 0xFE000000;
    default:
        return 0x20000000;
    }
}

#endif

#endif

/*
 * Copyright (c) 2023 Emilia Strange and the VegaOS team.
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

/* $Id$ */

#ifndef _SYS_MMIO_H_
#define _SYS_MMIO_H_

#include <sys/types.h>
#include <vm/vm.h>

/*
 * mmio_write<n> - Writes to MMIO address with specific size
 *
 * @addr: Address to write to.
 * @val: Value to write.
 *
 * These functions will add the higher half
 * offset (VM_HIGHER_HALF) if the MMIO address
 * is less than VM_HIGHER_HALF as it'll be safe
 * to assume it's a physical address. Page faults
 * from writes could be due to the resulting virtual
 * address not being mapped.
 */
#define _MMIO_WRITE_TYPE(TYPE, SUFFIX)                  \
    static inline void                                  \
    mmio_write##SUFFIX(void *addr, TYPE val)            \
    {                                                   \
        uintptr_t tmp;                                  \
                                                        \
        tmp = (uintptr_t)addr;                          \
        if (tmp < VM_HIGHER_HALF) {                     \
            tmp += VM_HIGHER_HALF;                      \
        }                                               \
        *(volatile TYPE *)tmp = val;                    \
    }

/*
 * mmio_read<n> - Does the same as mmio_write<n> but for reading
 *
 * @addr: Address to read from.
 */
#define _MMIO_READ_TYPE(TYPE, SUFFIX)                   \
    static inline TYPE                                  \
    mmio_read##SUFFIX(void *addr, TYPE val)             \
    {                                                   \
        uintptr_t tmp;                                  \
                                                        \
        tmp = (uintptr_t)addr;                          \
        if (tmp < VM_HIGHER_HALF) {                     \
            tmp += VM_HIGHER_HALF;                      \
        }                                               \
        return *(volatile TYPE *)tmp;                   \
    }

/*
 * To write to an MMIO address of, for example,
 * 8 bits, use mmio_write8(addr, val)
 */
_MMIO_WRITE_TYPE(uint8_t, 8)
_MMIO_WRITE_TYPE(uint16_t, 16)
_MMIO_WRITE_TYPE(uint32_t, 32)
#if __SIZEOF_SIZE_T__ == 8
_MMIO_WRITE_TYPE(uint64_t, 64)
#endif
__extension__

/*
 * To read from an MMIO address of, for example,
 * 8 bits, use mmio_read8(addr)
 */
_MMIO_READ_TYPE(uint8_t, 8)
_MMIO_READ_TYPE(uint16_t, 16)
_MMIO_READ_TYPE(uint32_t, 32)
#if __SIZEOF_SIZE_T__ == 8
_MMIO_READ_TYPE(uint64_t, 64)
#endif
__extension__

#endif  /* !_SYS_MMIO_H_ */

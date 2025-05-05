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

#ifndef _IF_ETHER_H_
#define _IF_ETHER_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <net/if.h>

#define MACADDR_LEN 6

struct __packed ether_frame {
    uint8_t sync[7];        /* Preamble (sync stuff) */
    uint8_t spd;            /* Start frame delimiter */
    uint8_t macd[6];        /* MAC destination */
    uint8_t macs[6];        /* MAC source */
    uint16_t type;          /* Protocol type */
    char payload[1];        /* sized @ 1+n */
};

/*
 * Used by the driver to buffer packets.
 */
struct etherbuf {
    struct ether_frame *frp;
    off_t head;
    off_t tail;
    size_t cap;     /* In entries */
};

/*
 * Ethernet device
 *
 * if_ether: E
 * driver: D
 *
 * @if_name: Interface name.
 * @tx: Transmit packets (D->E)
 */
struct etherdev {
    char if_name[IFNAMESIZ];
    struct etherbuf *buf;
    ssize_t(*tx)(struct etherdev *ep, const void *buf, size_t len);
    char mac_addr[MACADDR_LEN];
};

#endif  /* !_IF_ETHER_H_ */

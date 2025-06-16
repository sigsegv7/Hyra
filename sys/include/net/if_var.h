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

#ifndef _NET_IF_VAR_H_
#define _NET_IF_VAR_H_

#include <sys/queue.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/netbuf.h>

#define NETIF_ADDR_LEN 32   /* In bytes */

/* Return values for netif hooks */
#define NETIF_ENQ_OK       0        /* Enqueued */
#define NETIF_ENQ_FLUSHED  1        /* Internal queue flushed */

/* Interface types */
#define NETIF_TYPE_ANY  0   /* Any type */
#define NETIF_TYPE_WIRE 1   /* Ethernet */

/*
 * Represents the address of a network
 * interface.
 *
 * @data: Raw address bytes
 */
struct netif_addr {
    uint8_t data[NETIF_ADDR_LEN];
};

/*
 * Represents a network interface
 *
 * @name: Interface name
 * @type: Interface type (see NETIF_TYPE*)
 * @tx_enq: Enqueue a packet
 * @tx_start: Start a packet
 *
 * XXX: tx_enq() returns 0 on success and 1 if a flush was needed
 *      and the packets have been transmitted. Less than zero values
 *      indicate failure.
 */
struct netif {
    char name[IFNAMESIZ];
    uint8_t type;
    TAILQ_ENTRY(netif) link;
    struct netif_addr addr;
    int(*tx_enq)(struct netif *nifp, struct netbuf *nbp, void *data);
    void(*tx_start)(struct netif *nifp);
};

void netif_add(struct netif *nifp);
int netif_lookup(const char *name, uint8_t type, struct netif **res);

#endif  /* !_NET_IF_VAR_H_ */

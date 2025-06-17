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

#include <sys/types.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <vm/dynalloc.h>
#include <net/ethertypes.h>
#include <netinet/if_ether.h>
#include <string.h>

struct arp_pkt {
    struct ether_frame ehfr;
    struct ether_arp payload;
};

static struct arp_pkt *
arp_create(struct netif *nifp, uint32_t *sproto, uint32_t *tproto, uint16_t op)
{
    struct arp_pkt *packet;
    struct arp_hdr *hdrp;
    struct ether_frame *frp;
    struct ether_arp *payload;

    packet = dynalloc(sizeof(*packet));
    if (packet == NULL) {
        return NULL;
    }

    frp = &packet->ehfr;
    payload = &packet->payload;
    hdrp = &payload->hdr;

    /* Ethernet frame, from source to all */
    memcpy(frp->ether_saddr, &nifp->addr, ETHER_ADDR_LEN);
    memset(frp->ether_daddr, 0xFF, ETHER_ADDR_LEN);
    frp->ether_type = swap16(ETHERTYPE_ARP);

    /* Now for the ARP header */
    hdrp->hw_type = swap16(ARP_HWTYPE_ETHER);
    hdrp->proto_type = swap16(ETHERTYPE_IPV4);
    hdrp->hw_len = ETHER_ADDR_LEN;
    hdrp->proto_len = 4;
    hdrp->op_type = swap16(op);

    memcpy(payload->sha, frp->ether_saddr, ETHER_ADDR_LEN);
    memset(payload->tha, 0xFF, ETHER_ADDR_LEN);

    /* Protocol source address */
    *((uint32_t *)payload->spa) = *sproto;
    *((uint32_t *)payload->tpa) = *tproto;
    return packet;
}

int
arp_request(struct netif *nifp, uint8_t *sproto, uint8_t *tproto)
{
    struct arp_pkt *packet;
    struct netbuf nb;
    uint32_t *src_tmp, *targ_tmp;

    if (nifp->tx_enq == NULL) {
        return -ENOTSUP;
    }
    if (nifp->tx_start == NULL) {
        return -ENOTSUP;
    }

    src_tmp = (uint32_t *)sproto;
    targ_tmp = (uint32_t *)tproto;

    packet = arp_create(nifp, src_tmp, targ_tmp, ARP_REQUEST);
    if (packet == NULL) {
        return -ENOMEM;
    }

    nb.len = sizeof(*packet);
    memcpy(nb.data, packet, nb.len);

    nifp->tx_enq(nifp, &nb, NULL);
    nifp->tx_start(nifp);
    dynfree(packet);
    return 0;
}

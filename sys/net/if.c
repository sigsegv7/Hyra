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
#include <sys/queue.h>
#include <sys/spinlock.h>
#include <sys/errno.h>
#include <net/if_var.h>
#include <string.h>

static TAILQ_HEAD(, netif) netif_list;
static bool netif_init = false;

/*
 * Expose a network interface to the rest of the
 * system.
 */
void
netif_add(struct netif *nifp)
{
    if (!netif_init) {
        TAILQ_INIT(&netif_list);
        netif_init = true;
    }

    TAILQ_INSERT_TAIL(&netif_list, nifp, link);
}

/*
 * Lookup a network interface by name or type.
 *
 * @name: Name to lookup (use `type' if NULL)
 * @type: Type to lookup (use if `name' is NULL)
 */
int
netif_lookup(const char *name, uint8_t type, struct netif **res)
{
    struct netif *netif;

    if (!netif_init) {
        return -EAGAIN;
    }

    TAILQ_FOREACH(netif, &netif_list, link) {
        if (name != NULL) {
            if (strcmp(netif->name, name) == 0) {
                *res = netif;
                return 0;
            }
        }

        if (name == NULL && netif->type == type) {
            *res = netif;
            return 0;
        }
    }

    return -ENODEV;
}

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
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/driver.h>
#include <vm/dynalloc.h>
#include <string.h>

#define BLACKLIST_SIZE 64

/*
 * A driver blacklist entry
 *
 * @name: Name of driver to be blacklisted
 * @buckets: To handle collisions
 */
struct blacklist_entry {
    char *name;
    TAILQ_ENTRY(blacklist_entry) link;
    TAILQ_HEAD(, blacklist_entry) buckets;
};

static struct blacklist_entry blacklist[BLACKLIST_SIZE];

static uint32_t
fnv1_hash(const char *s)
{
    uint32_t hash = 2166136261UL;
    const uint8_t *p = (uint8_t *)s;

    while (*p != '\0') {
        hash ^= *p;
        hash = hash * 0x01000193;
        ++p;
    }

    return hash;
}

/*
 * Returns a bucket in case of collision
 */
static struct blacklist_entry *
blacklist_collide(struct blacklist_entry *entp, const char *name)
{
    struct blacklist_entry *tmp;

    if (entp->name == NULL) {
        return NULL;
    }

    TAILQ_FOREACH(tmp, &entp->buckets, link) {
        if (strcmp(name, tmp->name) == 0) {
            return tmp;
        }
    }

    return NULL;
}

/*
 * Mark a driver to be ignored during startup.
 * Blacklisted drivers will not be ran.
 *
 * @name: Name of driver (e.g., 'ahci')
 */
int
driver_blacklist(const char *name)
{
    struct blacklist_entry *ent;
    struct blacklist_entry *bucket;
    size_t name_len;
    uint32_t hash;

    if (name == NULL) {
        return -EINVAL;
    }

    hash = fnv1_hash(name);
    ent = &blacklist[hash % BLACKLIST_SIZE];
    if (ent->name != NULL) {
        bucket = dynalloc(sizeof(*bucket));
        if (bucket == NULL) {
            return -EINVAL;
        }
        TAILQ_INSERT_TAIL(&ent->buckets, bucket, link);
        return 0;
    }

    name_len = strlen(name);
    ent->name = dynalloc(name_len + 1);
    if (ent->name == NULL) {
        return -ENOMEM;
    }
    memcpy(ent->name, name, name_len + 1);
    return 0;
}

/*
 * Checks if a driver name is in the blacklist.
 * Returns 0 if not, otherwise 1.
 */
int
driver_blacklist_check(const char *name)
{
    struct blacklist_entry *ent;
    uint32_t hash;

    if (name == NULL) {
        return -EINVAL;
    }

    hash = fnv1_hash(name);
    ent = &blacklist[hash % BLACKLIST_SIZE];
    if (ent->name == NULL) {
        return 0;
    }

    if (strcmp(ent->name, name) == 0) {
        return 1;
    }

    ent = blacklist_collide(ent, name);
    if (ent != NULL) {
        return 1;
    }

    return 0;
}

/*
 * Initialize each entry in the driver
 * blacklist
 */
void
driver_blacklist_init(void)
{
    for (size_t i = 0; i < BLACKLIST_SIZE; ++i) {
        blacklist[i].name = NULL;
        TAILQ_INIT(&blacklist[i].buckets);
    }
}

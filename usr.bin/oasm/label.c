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
#include <oasm/label.h>
#include <oasm/log.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

static struct oasm_label *labels[MAX_LABELS];
static size_t label_count = 0;

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
 * The label table is a big hashmap containing
 * label entries. This function creates and add
 * a new label into the table.
 *
 * @name: Name of the label (e.g., _start)
 * @ip: Instruction pointer
 */
int
label_enter(const char *name, uintptr_t ip)
{
    uint32_t hash = fnv1_hash(name);
    uint32_t idx = hash % MAX_LABELS;
    struct oasm_label *lp, *lp_new;

    if (label_count >= MAX_LABELS) {
        oasm_err("[internal error]: too many labels\n");
        return -EIO;
    }

    lp_new = malloc(sizeof(*lp_new));
    if (lp_new == NULL) {
        oasm_err("[internal error]: out of memory\n");
        return -ENOMEM;
    }

    /* Initialize the label */
    lp_new->name = strdup(name);
    lp_new->ip = ip;
    TAILQ_INIT(&lp_new->buckets);

    /*
     * If there is no existing entry here, we
     * can take this slot.
     */
    lp = labels[idx];
    if (lp == NULL) {
        labels[idx] = lp_new;
        ++label_count;
        return 0;
    }

    /*
     * To prevent collisions in our table here,
     * we must check if the name matches at all.
     * If it does not, there is a collision and
     * we'll have to add this to a bucket.
     */
    if (strcmp(name, lp->name) != 0) {
        TAILQ_INSERT_TAIL(&lp->buckets, lp_new, link);
        ++label_count;
        return 0;
    }

    /* Can't put the same entry in twice */
    oasm_err("[internal error]: duplicate labels\n");
    return -EEXIST;
}

/*
 * Find a label entry in the label table.
 *
 * @name: Name of the label to lookup (e.g., _start)
 */
struct oasm_label *
label_lookup(const char *name)
{
    uint32_t hash = fnv1_hash(name);
    uint32_t idx = hash % MAX_LABELS;
    struct oasm_label *lp, *lp_tmp;

    lp = labels[idx];
    if (lp == NULL) {
        return NULL;
    }

    /* Is this the label we are looking up? */
    if (strcmp(name, lp->name) == 0) {
        return lp;
    }

    /* Maybe there was a collision? */
    TAILQ_FOREACH(lp_tmp, &lp->buckets, link) {
        if (strcmp(name, lp_tmp->name) == 0) {
            return lp_tmp;
        }
    }

    return NULL;
}

/*
 * Clean up all allocated labels by
 * calling free() on each entry of
 * the queue.
 */
void
labels_destroy(void)
{
    struct oasm_label *lp;

    for (size_t i = 0; i < MAX_LABELS; ++i) {
        lp = labels[i];
        if (lp != NULL) {
            free(lp->name);
            free(lp);
        }
    }
}

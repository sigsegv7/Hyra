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

#include <sys/vsr.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/limits.h>
#include <sys/syslog.h>
#include <vm/dynalloc.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("vsr: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

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
 * Add a VSR capsule to a domain.
 */
static void
vsr_domain_add(struct vsr_domain *vsp, struct vsr_capsule *cap)
{
    struct vsr_table *tab;
    struct vsr_capsule **slot;
    uint32_t hash;

    if (vsp == NULL || cap == NULL) {
        return;
    }

    if (cap->name == NULL) {
        pr_error("vsr_domain_add: cap->name == NULL\n");
        return;
    }

    tab = &vsp->table;
    hash = fnv1_hash(cap->name);
    slot = &tab->capsules[hash % VSR_MAX_CAPSULE];

    /* If this slot is free, set it */
    if (*slot == NULL) {
        *slot = cap;
        return;
    }

    /* Handle collision */
    TAILQ_INSERT_TAIL(&(*slot)->buckets, cap, link);
}

/*
 * Handle VSR domain hashmap collisions.
 *
 * @slot: Slot that we have collided with
 * @name: Name to lookup
 *
 * Returns the pointer to the actual capsule if the
 * collision has been resolved, otherwise, NULL if the
 * entry to look up was not found.
 */
static struct vsr_capsule *
vsr_domain_clash(struct vsr_capsule *slot, const char *name)
{
    struct vsr_capsule *cap_ent;

    TAILQ_FOREACH(cap_ent, &slot->buckets, link) {
        if (cap_ent == NULL) {
            continue;
        }

        if (strcmp(cap_ent->name, name) == 0) {
            return cap_ent;
        }
    }

    return NULL;
}

/*
 * Lookup a capsule within a VSR domain
 * by name.
 *
 * @vsp: Domain to lookup within
 * @name: Name to use as lookup key
 *
 * Returns NULL if no entry was found.
 */
static struct vsr_capsule *
vfs_domain_lookup(struct vsr_domain *vsp, const char *name)
{
    uint32_t hash;
    struct vsr_table *tab;
    struct vsr_capsule **slot;

    if (vsp == NULL || name == NULL) {
        return NULL;
    }

    tab = &vsp->table;
    hash = fnv1_hash(name);
    slot = &tab->capsules[hash % VSR_MAX_CAPSULE];

    if (*slot == NULL) {
        return NULL;
    }

    if (strcmp((*slot)->name, name) != 0) {
        return vsr_domain_clash(*slot, name);
    }

    return *slot;
}

/*
 * Destroy a VSR capsule
 *
 * @capule: Capsule to destroy
 */
static void
vsr_destroy_capsule(struct vsr_capsule *capsule)
{
    struct vsr_capsule *bucket;
    struct capsule_ops *ops;

    if (capsule->name != NULL) {
        dynfree(capsule->name);
        capsule->name = NULL;
    }

    ops = &capsule->ops;
    if (ops->reclaim != NULL) {
        ops->reclaim(capsule, 0);
    }

    TAILQ_FOREACH(bucket, &capsule->buckets, link) {
        if (bucket == NULL) {
            continue;
        }
        vsr_destroy_capsule(bucket);
    }

    /* Release any held locks */
    mutex_release(&capsule->lock);
}

/*
 * Destroy a VSR table
 *
 * @tab: Table to destroy.
 */
static void
vsr_destroy_table(struct vsr_table *tab)
{
    struct vsr_capsule *capsule;

    if (tab == NULL) {
        pr_error("vsr_destroy_table: tab is NULL\n");
        return;
    }

    for (int i = 0; i < VSR_MAX_CAPSULE; ++i) {
        if ((capsule = tab->capsules[i]) == NULL) {
            continue;
        }

        vsr_destroy_capsule(capsule);
    }
}

/*
 * Allocate a new VSR capsule and add it to
 * VSR domain.
 */
struct vsr_capsule *
vsr_new_capsule(vsr_domain_t type, const char *name)
{
    struct vsr_capsule *capsule;
    struct vsr_domain *domain;
    struct proc *td = this_td();

    /* Valid type? */
    if (type >= VSR_MAX_DOMAIN) {
        return NULL;
    }

    if (__unlikely(td == NULL)) {
        return NULL;
    }

    /*
     * The VSR domain must be registered for
     * us to add any capsules to it.
     */
    if ((domain = td->vsr_tab[type]) == NULL) {
        pr_error("VSR domain %d not registered\n", type);
        return NULL;
    }

    /* Allocate a new capsule */
    capsule = dynalloc(sizeof(*capsule));
    if (capsule == NULL) {
        return NULL;
    }

    memset(capsule, 0, sizeof(*capsule));
    capsule->name = strdup(name);

    TAILQ_INIT(&capsule->buckets);
    vsr_domain_add(domain, capsule);
    return capsule;
}

/*
 * Allocate a new VSR domain and add it to
 * the current process.
 *
 * @type: VSR type (e.g., VSR_FILE)
 */
struct vsr_domain *
vsr_new_domain(vsr_domain_t type)
{
    struct vsr_domain *domain;
    struct vsr_table *tablep;
    struct proc *td = this_td();

    /* Valid type? */
    if (type >= VSR_MAX_DOMAIN) {
        return NULL;
    }

    /*
     * The scheduler should be set up before any
     * calls to vsr_new_vec() should be made.
     */
    if (__unlikely(td == NULL)) {
        return NULL;
    }

    /*
     * Do not overwrite the entry if it is
     * already allocated and log this anomalous
     * activity.
     */
    if (td->vsr_tab[type] != NULL) {
        pr_error("[security]: type %d already allocated\n", type);
        return NULL;
    }

    domain = dynalloc(sizeof(*domain));
    if (domain == NULL) {
        return NULL;
    }

    /* Initialize the domain */
    memset(domain, 0, sizeof(*domain));
    domain->type = type;

    /* Initialize the domain's table */
    tablep = &domain->table;
    td->vsr_tab[type] = domain;
    return domain;
}

/*
 * Lookup a capsule by name for the current
 * process.
 */
struct vsr_capsule *
vsr_lookup_capsule(vsr_domain_t type, const char *name)
{
    struct vsr_domain *domain;
    struct proc *td = this_td();

    /* Must be on a process */
    if (__unlikely(td == NULL)) {
        return NULL;
    }

    /*
     * The VSR domain must be registered for
     * us to lookup any capsules from it.
     */
    if ((domain = td->vsr_tab[type]) == NULL) {
        pr_error("VSR domain %d not registered\n", type);
        return NULL;
    }

    return vfs_domain_lookup(domain, name);
}

/*
 * Initialize per-process domains
 */
void
vsr_init_domains(void)
{
    if (vsr_new_domain(VSR_FILE) == NULL) {
        pr_error("failed to initialize VSR file domain\n");
    }
}

/*
 * Destroy per-process domains
 */
void
vsr_destroy_domains(void)
{
    struct proc *td = this_td();
    struct vsr_domain *domain;

    if (__unlikely(td == NULL)) {
        return;
    }

    for (int i = 0; i < VSR_MAX_DOMAIN; ++i) {
        if ((domain = td->vsr_tab[i]) == NULL) {
            continue;
        }

        vsr_destroy_table(&domain->table);
    }
}

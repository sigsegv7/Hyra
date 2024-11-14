/*
 * Copyright (c) 2023-2024 Ian Marco Moffett and the Osmora Team.
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

#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/spinlock.h>
#include <vm/dynalloc.h>
#include <string.h>

#define VCACHE_SIZE 64

#define pr_trace(fmt, ...) kprintf("vcache: " fmt, ##__VA_ARGS__)

struct vcache {
    TAILQ_HEAD(vcache_head, vnode) q;
    ssize_t size;    /* In entries (-1 not set up) */
} vcache = { .size = -1 };

/*
 * Our vcache will be here if our caching type is
 * global.
 */
static int vcache_type = VCACHE_TYPE_NONE;
__cacheline_aligned static struct spinlock vcache_lock;

/*
 * Pull a vnode from the head of the global
 * vcache. Returns NULL if none are found.
 *
 * XXX: Caller must acquire vcache_lock.
 */
static struct vnode *
vcache_global_pull(void)
{
    struct vnode *vp;

    if (vcache.size <= 0) {
        return NULL;
    }

    vp = TAILQ_FIRST(&vcache.q);
    TAILQ_REMOVE(&vcache.q, vp, vcache_link);
    --vcache.size;
    return vp;
}

/*
 * Add a new entry to the global vcache
 *
 * XXX: Caller must acquire vcache_lock.
 * @vp: New vnode to add.
 */
static int
vcache_global_add(struct vnode *vp)
{
    struct vnode *tmp;

    /*
     * Do some checks on the vcache size, should be >= 0
     * but if it is -1 then we just need to initialize the
     * queue. However, if it is less than -1... Then shit,
     * good luck debugging I suppose.
     *
     * The global vcache naturally behaves as an LRU cache.
     * If we need more space, the tail of the queue is evicted.
     */
    if (vcache.size < 0) {
        TAILQ_INIT(&vcache.q);
        vcache.size = 0;
    } else if (vcache.size < -1) {
        panic("vcache_global_add: Bad vcache size, catching fire\n");
    } else if (vcache.size == VCACHE_SIZE) {
        /* Evict the tail */
        tmp = TAILQ_LAST(&vcache.q, vcache_head);
        TAILQ_REMOVE(&vcache.q, tmp, vcache_link);
        dynfree(tmp);
        --vcache.size;
    }

    TAILQ_INSERT_TAIL(&vcache.q, vp, vcache_link);
    ++vcache.size;
    return 0;
}

/*
 * Migrate the vnode cache (vcache) from one mode
 * (e.g., global, proc, none) to another. This transition
 * is done without an extreme performance impact through a
 * process called lazy vcache migration (LZVM). For example,
 * if we update the vcache type to be "proc" from an initial
 * type of "global", the global vcache is made read-only until
 * all entries are eventually invalidated naturally. In other
 * words, both the global vcache and per-process vcaches will
 * be checked during the migration process, however once the
 * global vcache becomes empty it will no longer be checked.
 */
int
vfs_vcache_migrate(int newtype)
{
    char *sysctl_val;
    struct sysctl_args args;
    int retval, name = KERN_VCACHE_TYPE;

    switch (newtype) {
    case VCACHE_TYPE_NONE:
        sysctl_val = "none";
        break;
    case VCACHE_TYPE_PROC:
        sysctl_val = "proc";
        break;
    case VCACHE_TYPE_GLOBAL:
        sysctl_val = "global";
        break;
    default:
        return -EINVAL;
    }

    /* Prepare the sysctl args */
    args.name = &name;
    args.nlen = 1;
    args.oldp = NULL;
    args.oldlenp = NULL;
    args.newp = sysctl_val;
    args.newlen = strlen(sysctl_val);

    if ((retval = sysctl(&args)) != 0) {
        return retval;
    }

    vcache_type = newtype;
    return 0;
}

/*
 * Add a vnode to vcache.
 *
 * @vp: Pointer of vnode to add.
 */
int
vfs_vcache_enter(struct vnode *vp)
{
    int retval = 0;

    switch (vcache_type) {
    case VCACHE_TYPE_NONE:
        break;
    case VCACHE_TYPE_PROC:
        /* TODO */
        pr_trace("warn: proc vcache not supported, using global...\n");
        vcache_type = VCACHE_TYPE_GLOBAL;
        /* - FALL THROUGH - */
    case VCACHE_TYPE_GLOBAL:
        spinlock_acquire(&vcache_lock);
        retval = vcache_global_add(vp);
        spinlock_release(&vcache_lock);
        break;
    default:
        pr_trace("warn: Bad vcache type, falling back to none\n");
        vcache_type = VCACHE_TYPE_NONE;
        break;
    }

    return retval;
}

/*
 * Pull a random vnode from the vcache to
 * recycle.
 */
struct vnode *
vfs_recycle_vnode(void)
{
    struct vnode *vp = NULL;

    switch (vcache_type) {
    case VCACHE_TYPE_NONE:
        break;
    case VCACHE_TYPE_PROC:
        /* TODO */
        pr_trace("warn: proc vcache not supported, using global...\n");
        vcache_type = VCACHE_TYPE_GLOBAL;
        /* - FALL THROUGH - */
    case VCACHE_TYPE_GLOBAL:
        spinlock_acquire(&vcache_lock);
        vp = vcache_global_pull();
        spinlock_release(&vcache_lock);
        break;
    default:
        pr_trace("warn: Bad vcache type, falling back to none\n");
        vcache_type = VCACHE_TYPE_NONE;
        break;
    }

    return vp;
}

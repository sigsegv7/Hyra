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

#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/spinlock.h>
#include <vm/dynalloc.h>
#include <string.h>

#define VCACHE_SIZE 64

#define pr_trace(fmt, ...) kprintf("vcache: " fmt, ##__VA_ARGS__)

/*
 * Our vcache will be here if our caching type is
 * global.
 */
static int vcache_type = VCACHE_TYPE_NONE;
static struct vcache vcache = { .size = -1 };
__cacheline_aligned static struct spinlock vcache_lock;

static inline int
vcache_proc_new(struct proc *td)
{
    struct vcache *vcp;

    vcp = dynalloc(sizeof(*vcp));
    if (vcp == NULL) {
        return -ENOMEM;
    }

    vcp->size = -1;
    td->vcache = vcp;
    return 0;
}

/*
 * Pull a vnode from the head of a vcache.
 * Returns NULL if none are found.
 */
static struct vnode *
vcache_pull(struct vcache *vcp)
{
    struct vnode *vp;

    if (vcp->size <= 0) {
        return NULL;
    }

    vp = TAILQ_FIRST(&vcp->q);
    TAILQ_REMOVE(&vcp->q, vp, vcache_link);
    --vcp->size;
    return vp;
}

/*
 * Add a new entry to a vcache
 *
 * @vp: New vnode to add.
 */
static int
vcache_add(struct vnode *vp, struct vcache *vcp)
{
    struct vnode *tmp;

    /*
     * Do some checks on the vcache size, should be >= 0
     * but if it is -1 then we just need to initialize the
     * queue. However, if it is less than -1... Then shit,
     * good luck debugging I suppose.
     *
     * Vcaches naturally behave as LRU caches. If we need
     * more space, the tail of the queue is evicted.
     */
    if (vcp->size < 0) {
        TAILQ_INIT(&vcp->q);
        vcp->size = 0;
    } else if (vcp->size < -1) {
        panic("vcache_add: Bad vcache size, catching fire\n");
    } else if (vcp->size == VCACHE_SIZE) {
        /* Evict the tail */
        tmp = TAILQ_LAST(&vcp->q, vcache_head);
        TAILQ_REMOVE(&vcp->q, tmp, vcache_link);
        dynfree(tmp);
        --vcp->size;
    }

    TAILQ_INSERT_TAIL(&vcp->q, vp, vcache_link);
    ++vcp->size;
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
    struct proc *td;
    int retval = 0;

    switch (vcache_type) {
    case VCACHE_TYPE_NONE:
        break;
    case VCACHE_TYPE_PROC:
        td = this_td();

        /* Create a new vcache if needed */
        if (td->vcache == NULL)
            retval = vcache_proc_new(td);
        if (retval != 0)
            return retval;

        spinlock_acquire(&td->vcache_lock);
        retval = vcache_add(vp, td->vcache);
        spinlock_release(&td->vcache_lock);
        break;
    case VCACHE_TYPE_GLOBAL:
        spinlock_acquire(&vcache_lock);
        retval = vcache_add(vp, &vcache);
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
    struct proc *td = this_td();
    struct vcache *vcp = &vcache;
    struct spinlock *vclp = &vcache_lock;
    struct vnode *vp = NULL;

    switch (vcache_type) {
    case VCACHE_TYPE_NONE:
        break;
    case VCACHE_TYPE_PROC:
        /* Have no vcache? Make one */
        if (td->vcache == NULL) {
            vcache_proc_new(td);
            break;
        }

        vcp = td->vcache;
        vclp = &td->vcache_lock;

        /*
         * For LZVM, if we still have entries within the
         * global vcache, drain it.
         */
        if (!TAILQ_EMPTY(&vcache.q)) {
            vcp = &vcache;
            vclp = &vcache_lock;
        }

        spinlock_acquire(vclp);
        vp = vcache_pull(vcp);
        spinlock_release(vclp);
        break;
    case VCACHE_TYPE_GLOBAL:
        vcp = &vcache;
        vclp = &vcache_lock;

        /*
         * If we've fully drained the local vcache during LZVM,
         * we can deallocate and disable it.
         */
        if (td->vcache != NULL) {
            if (TAILQ_EMPTY(&td->vcache->q)) {
                dynfree(td->vcache);
                td->vcache = NULL;
            }
        }

        /*
         * For LZVM, if the current process still has a vcache
         * despite us being in global mode, just pull an entry
         * from it instead.
         */
        if (td->vcache != NULL) {
            vclp = &td->vcache_lock;
            vcp = td->vcache;
        }

        spinlock_acquire(vclp);
        vp = vcache_pull(vcp);
        spinlock_release(vclp);
        break;
    default:
        pr_trace("warn: Bad vcache type, falling back to none\n");
        vcache_type = VCACHE_TYPE_NONE;
        break;
    }

    return vp;
}

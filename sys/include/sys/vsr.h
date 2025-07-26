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

#ifndef _SYS_VSR_H_
#define _SYS_VSR_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/limits.h>
#if defined(_KERNEL)
#include <sys/mutex.h>
#endif  /* _KERNEL */

struct proc;

#define VSR_FILE    0x00000000  /* Represented by file */

/*
 * Defines the access semantics of whether
 * r/w operations should be passed down to the
 * global state or soley affecting a per-process
 * shallow copy.
 */
typedef uint32_t vsr_mode_t;

/*
 * The Virtual System Resource namespace consists of
 * domains containing named "capsules". The domain is
 * simply a table indexed by a type value e.g. VSR_FILE
 * and a capsule is simply a structure containing global data
 * as well as a shallow copy which is controlled locally by the
 * process. The capsule also contains various access semantics
 * that help the VSR subsystem determine whether the access should
 * be passed down globally or virtualized locally within the process.
 */
typedef uint8_t vsr_domain_t;

/*
 * VSR mode bits
 */
#define VSR_GLOB_WRITE  BIT(0)   /* Writes are global */
#define VSR_GLOB_READ   BIT(1)   /* Reads are global */
#define VSR_GLOB_CRED   BIT(2)   /* Global for specific creds */

#if defined(_KERNEL)

struct vsr_capsule;

/*
 * VSR capsule operations
 *
 * @reclaim: Cleanup resources
 */
struct capsule_ops {
    int(*reclaim)(struct vsr_capsule *cap, int flags);
};

/*
 * Virtual system resource access
 * semantics.
 *
 * @glob: Global data
 * @shallow: Local per process copy
 * @mode: VSR mode (see VSR_GLOB_*)
 * @cred: Creds (used if VSR_GLOBAL_CRED set)
 */
struct vsr_access {
    void *glob;
    void *shallow;
    vsr_mode_t mode;
    struct ucred cred;
};

/*
 * A virtual system resource capsule containing
 * resource owner specific data and hashmap
 * buckets.
 *
 * @name: Capsule name (e.g., "consfeat"), must be freed
 * @data: Owner specific data
 * @shadow: Local shadow copy (per-process)
 * @buckets: Hashmap buckets
 * @link: Bucket link
 * @ops: Capsule operations
 * @lock: Mutex lock protecting fields
 */
struct vsr_capsule {
    char *name;
    void *data;
    void *shadow;
    TAILQ_HEAD(, vsr_capsule) buckets;
    TAILQ_ENTRY(vsr_capsule) link;
    struct capsule_ops ops;
    struct mutex lock;
};

/*
 * Virtual system resource table containg
 * VSRs for various types.
 *
 * Each VSR table belongs to a VSR domain
 * (e.g., VSR_FILE).
 *
 * @ncaps: Number of capsules
 * @is_init: Set if hashmap is set up
 * @capsules: VSR capsule hashmap
 */
struct vsr_table {
    struct vsr_capsule *capsules[VSR_MAX_CAPSULE];
};

/*
 * Virtual system resource domain (VSR).
 *
 * A VSR is represented by a specific VSR type
 * (see VSR_*). Each VSR has a table of VSR capsules
 * looked up by a VSR capsule name.
 *
 * One per process.
 *
 * @type: VSR type
 * @table: VSR table
 */
struct vsr_domain {
    int type;
    struct vsr_table table;
};

void vsr_init_domains(struct proc *td);
void vsr_destroy_domains(struct proc *td);

struct vsr_domain *vsr_new_domain(struct proc *td, vsr_domain_t type);
struct vsr_capsule *vsr_new_capsule(struct proc *td, vsr_domain_t type, const char *name);
struct vsr_capsule *vsr_lookup_capsule(struct proc *td, vsr_domain_t type, const char *name);

#endif  /* _KERNEL */
#endif  /* !_SYS_VSR_H_ */

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

#ifndef _SYS_SYSCTL_H_
#define _SYS_SYSCTL_H_

#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/syscall.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif
#include <sys/param.h>

/*
 * List of 'kern.* ' identifiers
 */
#define KERN_OSTYPE         0
#define KERN_OSRELEASE      1
#define KERN_VERSION        2
#define KERN_VCACHE_TYPE    3
#define KERN_HOSTNAME       4

/*
 * List of 'hw.* ' identifiers
 */
#define HW_PAGESIZE   5
#define HW_NCPU       6
#define HW_MACHINE    7

/*
 * Option types (i.e., int, string, etc) for
 * sysctl entries.
 *
 * TODO: Add SYSCTL_OPTYPE_NODE to define sysctl nodes in
 *       the hierarchy.
 */
#define SYSCTL_OPTYPE_INT_RO   0
#define SYSCTL_OPTYPE_STR_RO   1
#define SYSCTL_OPTYPE_INT      2
#define SYSCTL_OPTYPE_STR      3

#if defined(_KERNEL)
struct sysctl_entry {
    int enttype;
    int optype;
    void *data;
};

scret_t sys_sysctl(struct syscall_args *scargs);
int sysctl_clearstr(int name);
#endif  /* _KERNEL */

/*
 * The sysctl entries use an MIB (Management Information Base) style
 * naming scheme and follow a hierarchical naming structure. This is
 * similar to the structure described in RFC 3418 for the Simple
 * Network Management Protocol (SNMP).
 */
struct sysctl_args {
    int *name;
    int nlen;
    void *oldp;
    size_t *oldlenp;
    void *newp;
    size_t newlen;
};

int sysctl(struct sysctl_args *args);

#endif  /* !_SYS_SYSCTL_H_ */

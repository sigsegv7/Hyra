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
#include <sys/limine.h>
#include <sys/limits.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/spinlock.h>
#include <sys/sched.h>
#include <sys/atomic.h>
#include <machine/cpu.h>
#include <vm/dynalloc.h>
#include <assert.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("cpu_mp: " fmt, ##__VA_ARGS__)

extern struct proc g_proc0;
static volatile struct limine_smp_request g_smp_req = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

static volatile uint32_t ncpu_up = 1;
static struct cpu_info *ci_list[CPU_MAX];
static struct spinlock ci_list_lock = {0};

static void
ap_trampoline(struct limine_smp_info *si)
{
    struct cpu_info *ci;

    ci = dynalloc(sizeof(*ci));
    __assert(ci != NULL);
    memset(ci, 0, sizeof(*ci));

    cpu_startup(ci);
    spinlock_acquire(&ci_list_lock);
    ci_list[ncpu_up] = ci;
    spinlock_release(&ci_list_lock);

    atomic_inc_int(&ncpu_up);
    sched_enter();
    while (1);
}

struct cpu_info *
cpu_get(uint32_t index)
{
    if (index >= ncpu_up) {
        return NULL;
    }

    return ci_list[index];
}

uint32_t
cpu_count(void)
{
    return ncpu_up;
}

void
mp_bootstrap_aps(struct cpu_info *ci)
{
    struct limine_smp_response *resp = g_smp_req.response;
    struct limine_smp_info **cpus;
    size_t cpu_init_counter;
    uint32_t ncpu;

    /* Should not happen */
    __assert(resp != NULL);

    cpus = resp->cpus;
    ncpu = resp->cpu_count;
    cpu_init_counter = ncpu - 1;
    ci_list[0] = ci;

    if (resp->cpu_count == 1) {
        pr_trace("CPU has 1 core, no APs to bootstrap...\n");
        return;
    }

    pr_trace("bootstrapping %d cores...\n", cpu_init_counter);
    for (size_t i = 0; i < resp->cpu_count; ++i) {
        if (ci->apicid == cpus[i]->lapic_id) {
            pr_trace("skip %d (BSP)... continue\n", ci->apicid);
            continue;
        }

        cpus[i]->goto_address = ap_trampoline;
    }

    /* Start up idle threads */
    pr_trace("kicking %d idle threads...\n", ncpu);
    for (uint32_t i = 0; i < ncpu; ++i) {
        spawn(&g_proc0, sched_enter, NULL, 0, NULL);
    }

    /* Wait for all cores to be ready */
    while ((ncpu_up - 1) < cpu_init_counter);
}

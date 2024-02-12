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

#include <machine/cpu_mp.h>
#include <sys/machdep.h>
#include <sys/limine.h>
#include <sys/types.h>
#include <sys/syslog.h>
#include <sys/sched.h>
#include <sys/cpu.h>
#include <vm/dynalloc.h>
#include <assert.h>

__MODULE_NAME("cpu_mp");
__KERNEL_META("$Hyra$: cpu_mp.c, Ian Marco Moffett, "
              "SMP related code");

static volatile struct limine_smp_request g_smp_req = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

static bool is_mp_supported = false;

static void
ap_create_cctx(struct cpu_info *ci)
{
    struct cpu_ctx *cctx;

    cctx = dynalloc(sizeof(struct cpu_ctx));
    __assert(cctx != NULL);

    cctx->ci = ci;

    /* Set *our* %GS now... */
    wrmsr(IA32_KERNEL_GS_BASE, (uintptr_t)cctx);

    if (!is_mp_supported) {
        is_mp_supported = true;
    }
}

static void
ap_trampoline(struct limine_smp_info *si)
{
    struct cpu_info *ci;

    ci = (void *)si->extra_argument;

    pre_init();
    ap_create_cctx(ci);
    processor_init();

    sched_init_processor(ci);

    /* Should not be reached */
    __assert(0);
    __builtin_unreachable();
}

/*
 * Returns true if SMP is supported.
 */
bool
mp_supported(void)
{
    return is_mp_supported;
}

void
ap_bootstrap(struct cpu_info *ci)
{
    struct limine_smp_response *resp = g_smp_req.response;
    struct limine_smp_info **cpus;
    struct cpu_info *ap_ci = NULL;
    size_t cpu_init_counter;

    /* Should not happen */
    __assert(resp != NULL);

    cpus = resp->cpus;
    cpu_init_counter = resp->cpu_count - 1;

    if (resp->cpu_count == 1) {
        /* TODO: Allow single core processing */
        panic("System only has 1 core!\n");
    }

    /* Create processor context */
    ap_create_cctx(ci);

    KINFO("Bootstrapping %d cores...\n", cpu_init_counter);
    for (size_t i = 0; i < resp->cpu_count; ++i) {
        if (ci->id == cpus[i]->lapic_id) {
            KINFO("Skip %d (BSP)... continue\n", ci->id);
            continue;
        }

        ap_ci = dynalloc(sizeof(struct cpu_info));
        __assert(ap_ci != NULL);
        cpu_attach(ap_ci);

        cpus[i]->extra_argument = (uintptr_t)dynalloc(sizeof(struct cpu_info));
        cpus[i]->goto_address = ap_trampoline;
    }
    KINFO("AP bootstrap done for all cores\n");
}

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

#include <sys/cpu.h>
#include <sys/types.h>
#include <sys/panic.h>
#include <sys/machdep.h>
#include <vm/dynalloc.h>
#include <assert.h>

__MODULE_NAME("kern_cpu");
__KERNEL_META("$Hyra$: kern_cpu.c, Ian Marco Moffett, "
              "Machine independent CPU interface");

#define CI_LIST_SZ \
    sizeof(struct cpu_info *) * (MAXCPUS + 1)

static size_t ncpu = 0;
static struct cpu_info **ci_list = NULL;

void
cpu_attach(struct cpu_info *ci)
{
    if ((ci->idx = ncpu++) >= (MAXCPUS + 1)) {
        panic("Machine core count exceeds MAXCPUS!\n");
    }

    if (ci_list == NULL) {
        ci_list = dynalloc(CI_LIST_SZ);
        __assert(ci_list != NULL);
    }

    ci_list[cpu_index(ci)] = ci;
}

struct cpu_info *
cpu_get(size_t i)
{
    if (i >= ncpu || ci_list == NULL) {
        return NULL;
    }

    return ci_list[i];
}

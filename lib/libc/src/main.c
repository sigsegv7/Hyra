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

#include <sys/auxv.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, const char **argv, const char **envp);

static uint64_t auxv[AT_MAX_COUNT];
FILE *stdout;

unsigned long
auxv_entry(unsigned long type)
{
    if (type >= AT_MAX_COUNT)
        return 0;

    return auxv[type];
}

int
__libc_entry(uint64_t *ctx)
{
    uint64_t envc = 0;
    uint64_t argc = *ctx;
    const char **argv = (const char **)(ctx + 1);
    const char **envp = (const char **)(argv + argc + 1);
    const struct auxv_entry *auxvp = NULL;

    /* Count envp entries */
    for (const char **env = envp; *env != NULL; ++env) {
        ++envc;
    }

    stdout = fdopen(STDOUT_FILENO, "a");
    auxvp = (struct auxv_entry *)(envp + envc + 1);

    /* Populate 'auxv' */
    while (auxvp->tag != AT_NULL) {
        if (auxvp->tag < AT_MAX_COUNT) {
            auxv[auxvp->tag] = auxvp->val;
        }
        ++auxvp;
    }

    return main(argc, argv, envp);
}

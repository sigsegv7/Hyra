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

#include <sys/exec.h>
#include <stdint.h>
#include <stddef.h>

extern int __libc_stdio_init(void);
extern int __malloc_mem_init(void);
uint64_t __libc_auxv[_AT_MAX];

int main(int argc, char **argv);

struct auxv_entry {
    uint64_t tag;
    uint64_t val;
};

int
__libc_entry(uint64_t *ctx)
{
    const struct auxv_entry *auxvp;
    int status;
    uint64_t argc, envc, tag;
    char **argv;
    char **envp;

    argc = *ctx;
    argv = (char **)(ctx + 1);
    envp = (char **)(argv + argc + 1);

    envc = 0;
    while (envp[envc] != NULL) {
        ++envc;
    }

    auxvp = (void *)(envp + envc + 1);
    for (int i = 0; i < _AT_MAX; ++i) {
        if (auxvp->tag == AT_NULL) {
            break;
        }
        if (auxvp->tag < _AT_MAX) {
            __libc_auxv[auxvp->tag] = auxvp->val;
        }

        ++auxvp;
    }

    if ((status = __libc_stdio_init()) != 0) {
        return status;
    }

    __malloc_mem_init();
    return main(argc, argv);
}

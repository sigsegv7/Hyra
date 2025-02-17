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

#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#include <sys/types.h>

#if defined(_KERNEL)

/* Danger: Do not change these !! */
#define AT_NULL 0
#define AT_ENTRY 1
#define AT_PHDR 2
#define AT_PHENT 3
#define AT_PHNUM 4
#define AT_EXECPATH 5
#define AT_SECURE 6
#define AT_RANDOM 7
#define AT_EXECFN 8
#define AT_PAGESIZE 9

#define MAX_PHDRS 32
#define STACK_PUSH(PTR, VAL) *(--(PTR)) = VAL
#define AUXVAL(PTR, TAG, VAL)   \
    STACK_PUSH(PTR, VAL);       \
    STACK_PUSH(PTR, TAG);

struct proc;

struct exec_range {
    paddr_t start;
    paddr_t end;
    vaddr_t vbase;
};

struct auxval {
    uint64_t at_entry;
    uint64_t at_phdr;
    uint64_t at_phent;
    uint64_t at_phnum;
};

/* A loaded program */
struct exec_prog {
    struct exec_range loadmap[MAX_PHDRS];
    struct auxval auxval;
    char **argp;
    char **envp;
    vaddr_t start;
    vaddr_t end;
};

struct execve_args {
    const char *pathname;
    char **argv;
    char **envp;
};

int execve(struct proc *td, const struct execve_args *args);
int elf64_load(const char *pathname, struct proc *td, struct exec_prog *prog);

void elf_unload(struct proc *td, struct exec_prog *prog);
void setregs(struct proc *td, struct exec_prog *prog, uintptr_t stack);

#endif  /* _KERNEL */
#endif  /* !_SYS_EXEC_H_ */

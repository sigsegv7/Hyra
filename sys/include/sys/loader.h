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

#ifndef _SYS_LOADER_H_
#define _SYS_LOADER_H_

#include <sys/types.h>
#include <vm/pmap.h>
#include <vm/vm.h>

/* DANGER!: DO NOT CHANGE THESE DEFINES */
#define AT_NULL 0
#define AT_ENTRY 1
#define AT_PHDR 2
#define AT_PHENT 3
#define AT_PHNUM 4
#define AT_EXECPATH 5
#define AT_SECURE 6
#define AT_RANDOM 7
#define AT_EXECFN 8

#define STACK_PUSH(ptr, val) *(--ptr) = val
#define AUXVAL(ptr, tag, val) __extension__ ({  \
    STACK_PUSH(ptr, val);                       \
    STACK_PUSH(ptr, tag);                       \
});

/* Auxiliary Vector */
struct auxval {
    uint64_t at_entry;
    uint64_t at_phdr;
    uint64_t at_phent;
    uint64_t at_phnum;
};

#if defined(_KERNEL)

int loader_load(struct vas vas, const void *dataptr, struct auxval *auxv,
                size_t load_base, char **ld_path, struct vm_range *prog_range);

#endif  /* defined(_KERNEL) */
#endif  /* !_SYS_LOADER_H_ */

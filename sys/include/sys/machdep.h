/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

/* $Id$ */

#ifndef _SYS_MACHDEP_H_
#define _SYS_MACHDEP_H_

#include <sys/types.h>
#if defined(_KERNEL)
#include <machine/gdt.h>
#endif      /* defined(_KERNEL) */

#if defined(_KERNEL)

/*
 * Arch specifics go here
 * along with an #if defined(...)
 *
 * XXX: When porting more architectures this
 *      may get messy. Figure out a way to
 *      seperate this into a different header.
 */
struct processor_machdep {
#if defined(__x86_64__)
    struct gdtr      *gdtr;
    struct gdt_entry *gdt;
#endif      /* defined(__x86_64__) */
};

/*
 * Sets arch specifics to their
 * defaults.
 */
#if defined(__x86_64__)
#define DEFAULT_PROCESSOR_MACHDEP           \
            {                               \
                .gdtr = &g_early_gdtr,      \
                .gdt  = &g_dmmy_gdt[0]      \
            }
#endif      /* defined(__x86_64__) */

struct processor {
    struct processor_machdep machdep;
};

__weak void processor_init(struct processor *processor);
__weak void interrupts_init(struct processor *processor);

void processor_halt(void);

#endif  /* defined(_KERNEL) */
#endif  /* !_SYS_MACHDEP_H_ */

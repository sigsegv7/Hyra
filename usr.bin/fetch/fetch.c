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

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CPUID(level, a, b, c, d)                        \
    __ASMV("cpuid\n\t"                                  \
            : "=a" (a), "=b" (b), "=c" (c), "=d" (d)    \
            : "0" (level))

#define ASCII_ART \
    "  ____      \n" \
    " | \\__\\      user: %s\n" \
    " | /\\  \\     OS:   Hyra/amd64 v"_OSVER"\n" \
    " |/  \\  \\    arch: "_OSARCH"\n" \
    " \\ R. \\  \\   cpu: %s\n" \
    "  \\ I. \\  \\\n"


/*
 * Get the processor brand string
 *
 * @buffer: Buffer to copy branch string
 *
 * Returns a pointer to newly allocated memory
 * containing the vendor string. One must ensure
 * to call free() after use.
 */
static char *
get_brand(void)
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t regs[12];
    char buf[sizeof(regs) + 1];
    char *p = buf;

    /* Can we even get the brand? */
    CPUID(0x80000000, eax, ebx, ecx, edx);
    if (eax < 0x80000004) {
        return NULL;
    }

    CPUID(0x80000002, regs[0], regs[1], regs[2], regs[3]);
    CPUID(0x80000003, regs[4], regs[5], regs[6], regs[7]);
    CPUID(0x80000004, regs[8], regs[9], regs[10], regs[11]);

    /* Log it */
    memcpy(p, regs, sizeof(regs));
    buf[sizeof(regs)] = '\0';

    /* Strip away leading whitespaces */
    for (int i = 0; i < sizeof(buf); ++i) {
        if (buf[i] == ' ') {
            ++p;
        } else {
            break;
        }
    }

    return strdup(p);
}

int
main(void)
{
    char *brand = get_brand();

    if (brand == NULL) {
        brand = strdup("unknown");
    }

    printf(ASCII_ART, getlogin(), brand);
    free(brand);
    return 0;
}

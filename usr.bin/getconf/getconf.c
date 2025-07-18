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

#include <sys/param.h>
#include <sys/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

struct sysvar {
    const char *var;
    uint8_t auxv : 1;
    uint32_t val;
};

static struct sysvar vartab[] = {
    { "PAGESIZE", 1, AT_PAGESIZE },
    { "CHAR_BIT", 0, CHAR_BIT },
    { "NAME_MAX", 0, NAME_MAX },
    { "PATH_MAX", 0, PATH_MAX },
    { "SSIZE_MAX", 0, SSIZE_MAX },
    { NULL, 0, 0 }
};

static int
getvar_val(struct sysvar *vp)
{
    if (vp->auxv) {
        return sysconf(vp->val);
    }

    return vp->val;
}

static int
getvar(const char *sysvar)
{
    for (int i = 0; vartab[i].var != NULL; ++i) {
        if (strcmp(vartab[i].var, sysvar) == 0) {
            return getvar_val(&vartab[i]);
        }
    }

    return -1;
}

int
main(int argc, char **argv)
{
    char *var;
    int retval;

    if (argc < 2) {
        printf("usage: getconf <SYSTEM VAR>\n");
        return -1;
    }

    var = argv[1];
    if ((retval = getvar(var)) < 0) {
        printf("bad system var \"%s\"\n", var);
        return retval;
    }

    printf("%d\n", retval);
    return 0;
}

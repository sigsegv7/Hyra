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

#include <sys/sysctl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NAME_OSTYPE         "ostype"
#define NAME_OSRELEASE      "osrelease"
#define NAME_VERSION        "version"
#define NAME_VCACHE_TYPE    "vcache_type"

/*
 * Convert string name to a sysctl name
 * definition.
 *
 * @name: Name to convert
 *
 *                Convert to int def
 *               /
 *    kern.ostype
 *         ^^ name
 *
 *  --
 *  Returns a less than zero value on failure
 *  (e.g., entry not found).
 */
static int
name_to_def(const char *name)
{
    switch (*name) {
    case 'v':
        if (strcmp(name, NAME_VERSION) == 0) {
            return KERN_VERSION;
        }

        if (strcmp(name, NAME_VCACHE_TYPE) == 0) {
            return KERN_VCACHE_TYPE;
        }

        return -1;
    case 'o':
        if (strcmp(name, NAME_OSTYPE) == 0) {
            return KERN_OSTYPE;
        }

        if (strcmp(name, NAME_OSRELEASE) == 0) {
            return KERN_OSRELEASE;
        }

        return -1;
    }

    return -1;
}

int
main(int argc, char **argv)
{
    struct sysctl_args args;
    char *var, *p;
    int type, name, error;
    size_t oldlen;
    char buf[128];

    if (argc < 2) {
        printf("sysctl: usage: sysctl <var>\n");
        return -1;
    }

    var = argv[1];
    p = strtok(var, ".");

    if (p == NULL) {
        printf("sysctl: bad var\n");
        return -1;
    }

    /* TODO: Non kern.* vars */
    if (strcmp(p, "kern") != 0) {
        printf("unknown var \"%s\"\n", p);
        return -1;
    }

    p = strtok(NULL, ".");
    if (p == NULL) {
        printf("sysctl: bad var \"%s\"\n", p);
        return -1;
    }

    if ((name = name_to_def(p)) < 0) {
        printf("sysctl: bad var \"%s\"\n", p);
        return name;
    }

    name = name;
    oldlen = sizeof(buf);
    args.name = &name;
    args.nlen = 1;
    args.oldp = buf;
    args.oldlenp = &oldlen;
    args.newp = NULL;
    args.newlen = 0;

    if ((error = sysctl(&args)) != 0) {
        printf("sysctl returned %d\n", error);
        return error;
    }

    printf("%s\n", buf);
    return 0;
}

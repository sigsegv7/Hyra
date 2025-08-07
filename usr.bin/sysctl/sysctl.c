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
#include <stdbool.h>

#define BUF_SIZE 128

/* Kern var string constants */
#define NAME_OSTYPE         "ostype"
#define NAME_OSRELEASE      "osrelease"
#define NAME_VERSION        "version"
#define NAME_VCACHE_TYPE    "vcache_type"

/* Hw var string constants */
#define NAME_PAGESIZE "pagesize"
#define NAME_NCPU     "ncpu"

/* Name start string constants */
#define NAME_KERN "kern"
#define NAME_HW   "hw"

/* Name start int constants */
#define NAME_DEF_KERN 0
#define NAME_DEF_HW   1

/*
 * Print the contents read from a sysctl
 * variable depending on its type.
 *
 * @data: Data to print
 * @is_str: True if a string
 */
static inline void
varbuf_print(char data[BUF_SIZE], bool is_str)
{
    uint32_t *val;

    if (is_str) {
        printf("%s\n", data);
    } else {
        val = (uint32_t *)data;
        printf("%d\n", *val);
    }
}


/*
 * Convert string name to a internal name
 * definition.
 *
 * @name: Name to convert
 *
 *                Convert to int def
 *               /
 *    kern.ostype
 *    ^^
 *
 *  --
 *  Returns a less than zero value on failure
 *  (e.g., entry not found).
 */
static int
name_to_def(const char *name)
{
    switch (*name) {
    case 'k':
        if (strcmp(name, NAME_KERN) == 0) {
            return NAME_DEF_KERN;
        }

        return -1;
    case 'h':
        if (strcmp(name, NAME_HW) == 0) {
            return NAME_DEF_HW;
        }

        return -1;
    }

    return -1;
}

/*
 * Handle parsing of 'kern.*' node names
 *
 * @node: Node name to parse
 * @is_str: Set to true if string
 */
static int
kern_node(const char *node, bool *is_str)
{
    switch (*node) {
    case 'v':
        if (strcmp(node, NAME_VERSION) == 0) {
            return KERN_VERSION;
        }

        if (strcmp(node, NAME_VCACHE_TYPE) == 0) {
            return KERN_VCACHE_TYPE;
        }
        return -1;
    case 'o':
        if (strcmp(node, NAME_OSTYPE) == 0) {
            return KERN_OSTYPE;
        }

        if (strcmp(node, NAME_OSRELEASE) == 0) {
            return KERN_OSRELEASE;
        }
        return -1;
    }

    return -1;
}

/*
 * Handle parsing of 'hw.*' node names
 *
 * @node: Node name to parse
 * @is_str: Set to true if string
 */
static int
hw_node(const char *node, bool *is_str)
{
    switch (*node) {
    case 'p':
        if (strcmp(node, NAME_PAGESIZE) == 0) {
            *is_str = false;
            return HW_PAGESIZE;
        }

        return -1;
    case 'n':
        if (strcmp(node, NAME_NCPU) == 0) {
            *is_str = false;
            return HW_NCPU;
        }

        return -1;
    }

    return -1;
}

/*
 * Convert string node to a sysctl name
 * definition.
 *
 * @name: Name to convert
 * @is_str: Set to true if string
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
node_to_def(int name, const char *node, bool *is_str)
{
    int retval;
    bool dmmy;

    /*
     * If the caller did not set `is_str' just
     * set it to a dummy value. Otherwise, we will
     * make it *default* to a 'true' value.
     */
    if (is_str == NULL) {
        is_str = &dmmy;
    } else {
        *is_str = true;
    }

    switch (name) {
    case NAME_DEF_KERN:
        return kern_node(node, is_str);
    case NAME_DEF_HW:
        return hw_node(node, is_str);
    }

    return -1;
}

int
main(int argc, char **argv)
{
    struct sysctl_args args;
    char *var, *p;
    int type, error;
    int root, name;
    size_t oldlen;
    bool is_str;
    char buf[BUF_SIZE];

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

    if ((root = name_to_def(p)) < 0) {
        printf("sysctl: bad var \"%s\"", p);
        return root;
    }

    p = strtok(NULL, ".");
    if (p == NULL) {
        printf("sysctl: bad var \"%s\"\n", p);
        return -1;
    }

    if ((name = node_to_def(root, p, &is_str)) < 0) {
        printf("sysctl: bad var \"%s\"\n", p);
        return name;
    }

    memset(buf, 0, sizeof(buf));
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

    varbuf_print(buf, is_str);
    return 0;
}

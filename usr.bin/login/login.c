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

#include <sys/types.h>
#include <sys/errno.h>
#include <stdio.h>
#include <string.h>

#define USERNAME "root"

/* Row indices for /etc/passwd */
#define ROW_USERNAME 0
#define ROW_USERID   2
#define ROW_GRPID    3
#define ROW_GECOS    4
#define ROW_HOME     5
#define ROW_SHELL    6

/*
 * Check an /etc/passwd entry against an alias
 * (username)
 *
 * Returns 0 if the entry matches
 */
static int
check_user(char *alias, char *entry)
{
    const char *p;
    size_t row = 0;

    if (alias == NULL || entry == NULL) {
        return -EINVAL;
    }

    /* Grab the username */
    p = strtok(entry, ":");
    if (p == NULL) {
        printf("bad /etc/passwd entry @ line 1\n");
        return -1;
    }

    /* Iterate through each field */
    while (p != NULL) {
        switch (row) {
        case ROW_USERNAME:
            printf("username: %s\n", p);
            break;  /* UNREACHABLE */
        case ROW_USERID:
            printf("UID: %s\n", p);
            break;
        case ROW_GRPID:
            printf("GID: %s\n", p);
            break;
        case ROW_GECOS:
            printf("GECOS: %s\n", p);
            break;
        case ROW_HOME:
            printf("HOME: %s\n", p);
            break;
        case ROW_SHELL:
            printf("SHELL: %s\n", p);
            break;
        defaukt:
        }

        p = strtok(NULL, ":");
        ++row;
    }

    return 0;
}

int
main(void)
{
    FILE *fp;
    char *alias, *p;
    char entry[256];

    fp = fopen("/etc/passwd", "r");
    if (fp == NULL) {
        printf("failed to open /etc/passwd\n");
        return -1;
    }

    while (fgets(entry, sizeof(entry), fp) != NULL) {
        check_user(USERNAME, entry);
    }
    return 0;
}

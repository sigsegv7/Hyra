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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define UNKNOWN_USER "unknown"

static char *ucache = NULL;

static int
match_entry(uid_t uid, char *entry)
{
    char uidstr[16];
    static char *username = NULL;
    char *p;
    size_t len;
    uint8_t row = 0;

    if (itoa(uid, uidstr, 10) == NULL) {
        return -1;
    }

    p = strtok(entry, ":");
    if (p == NULL) {
        return -1;
    }

    while (p != NULL) {
        switch (row) {
        case 0:
            username = p;
            break;
        case 2:
            /*
             * If the user ID matches, we'll cache the
             * username.
             */
            if (strcmp(uidstr, p) == 0) {
                ucache = username;
                return 0;
            }
            break;
        }

        p = strtok(NULL, ":");
        ++row;
    }

    return -1;
}

char *
getlogin(void)
{
    FILE *fp;
    char entry[256];
    int retval;
    uid_t uid = getuid();

    /* Get the user from the cache */
    if (ucache != NULL) {
        return ucache;
    }

    fp = fopen("/etc/passwd", "r");
    if (fp == NULL) {
        return UNKNOWN_USER;
    }

    while (fgets(entry, sizeof(entry), fp) != NULL) {
        if (match_entry(uid, entry) == 0) {
            fclose(fp);
            return ucache;
        }
    }

    fclose(fp);
    return UNKNOWN_USER;
}

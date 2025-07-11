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

#include <sys/spawn.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <crypto/sha256.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Row indices for /etc/passwd */
#define ROW_USERNAME 0
#define ROW_HASH     1
#define ROW_USERID   2
#define ROW_GRPID    3
#define ROW_GECOS    4
#define ROW_HOME     5
#define ROW_SHELL    6

#define is_ascii(C) ((C) >= 0 && (C) <= 128)
#define is_digit(C) ((C >= '0' && C <= '9'))

#define DEFAULT_SHELL "/usr/bin/osh"

static char buf[64];
static uint8_t buf_i;
static short echo_chars = 1;

/*
 * Verify a UID is valid
 *
 * Returns 0 on success
 */
static int
check_uid(const char *uid)
{
    size_t len;

    len = strlen(uid);

    /* Must not be greater than 4 chars */
    if (len > 4) {
        return -1;
    }

    for (int i = 0; i < len; ++i) {
        if (!is_digit(uid[i])) {
            return -1;
        }
    }

    return 0;
}

/*
 * Check an /etc/passwd entry against an alias
 * (username)
 *
 * @alias: Alias to lookup
 * @hash: Password hash
 * @entry: /etc/passwd entry
 *
 * Returns -1 on failure
 * Returns 0 if the entry matches
 */
static int
check_user(char *alias, char *hash, char *entry)
{
    const char *p;
    char *shell_argv[] = { DEFAULT_SHELL, NULL };
    char *envp[] = { NULL };
    size_t row = 0;
    size_t line = 1;
    short have_user = 0;
    short have_pw = 0;
    short have_uid = 0;
    uid_t uid = -1;

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
            if (strcmp(p, alias) == 0) {
                have_user = 1;
            }
            break;  /* UNREACHABLE */
        case ROW_HASH:
            if (strcmp(p, hash) == 0) {
                have_pw = 1;
            }
            break;
        case ROW_USERID:
            if (check_uid(p) != 0) {
                printf("bad uid @ line %d\n", line);
                return -1;
            }

            uid = atoi(p);
            have_uid = 1;
            break;
        case ROW_SHELL:
            /* TODO */
            break;
        }

        p = strtok(NULL, ":");
        ++row;
        ++line;
    }

    /*
     * We need to have found the password hash,
     * the username, AND the UID. If we have not,
     * then this has failed.
     */
    if (!have_pw || !have_user || !have_uid) {
        return -1;
    }

    setuid(uid);
    spawn(shell_argv[0], shell_argv, envp, SPAWN_WAIT);
    return 0;
}

static char *
getstr(void)
{
    char c, printc;
    int input;

    buf_i = 0;

    for (;;) {
        if ((input = getchar()) < 0) {
            continue;
        }

        c = (char)input;
        if (c == '\t') {
            continue;
        }

        /*
         * If we want to echo characters, 'printc' becomes
         * exactly the character we got. Otherwise, just
         * print little stars to redact it.
         */
        printc = echo_chars ? c : '*';

        /* return on newline */
        if (c == '\n') {
            buf[buf_i] = '\0';
            putchar('\n');
            return buf;
        }

        /* handle backspaces and DEL */
        if (c == '\b' || c == 127) {
            if (buf_i > 0) {
                fputs("\b \b", stdout);
                buf[--buf_i] = '\0';
            }
        } else if (is_ascii(c) && buf_i < sizeof(buf) - 1) {
            /* write to fd and add to buffer */
            buf[buf_i++] = c;
            putchar(printc);
        }
    }
}

static int
getuser(FILE *fp)
{
    char *pwtmp, *alias, *p;
    char entry[256];
    char pwhash[SHA256_HEX_SIZE];
    int retval;

    printf("username: ");
    p = getstr();
    alias = strdup(p);

    /* Grab the password now */
    echo_chars = 0;
    printf("password: ");
    p = getstr();
    pwtmp = strdup(p);
    sha256_hex(pwtmp, strlen(pwtmp), pwhash);

    /* Paranoia */
    memset(pwtmp, 0, strlen(pwtmp));
    buf_i = 0;
    memset(buf, 0, sizeof(buf));

    /* Clean up */
    free(pwtmp);
    pwtmp = NULL;

    /* See if anything matches */
    while (fgets(entry, sizeof(entry), fp) != NULL) {
        retval = check_user(alias, pwhash, entry);
        if (retval == 0) {
            printf("login: successful\n");
            free(alias);
            return 0;
        }
    }

    /* If we reach this point, bad creds */
    free(alias);
    alias = NULL;

    printf("bad username or password\n");
    fseek(fp, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    buf_i = 0;
    echo_chars = 1;
    return -1;
}

int
main(void)
{
    FILE *fp;

    fp = fopen("/etc/passwd", "r");
    if (fp == NULL) {
        printf("failed to open /etc/passwd\n");
        return -1;
    }

    printf("- Please authenticate yourself -\n");
    for (;;) {
        if (getuser(fp) == 0) {
            break;
        }
    }

    fclose(fp);
    return 0;
}

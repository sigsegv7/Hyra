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
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define SHELL_PATH   "/usr/bin/osh"
#define LOGIN_PATH   "/usr/bin/login"
#define INIT_RC_PATH "/usr/rc/init.rc"

static void
init_hostname(void)
{
    char hostname[128];
    int error;
    size_t len;
    FILE *fp;

    fp = fopen("/etc/hostname", "r");
    if (fp == NULL) {
        printf("[init]: error opening /etc/hostname\n");
        return;
    }

    error = fread(hostname, sizeof(char), sizeof(hostname), fp);
    if (error <= 0) {
        printf("[init]: error reading /etc/hostname\n");
        fclose(fp);
        return;
    }

    len = strlen(hostname);
    hostname[len - 2] = '\0';

    if (sethostname(hostname, len) < 0) {
        printf("[init]: error setting hostname\n");
        printf("[init]: tried to set %s (len=%d)\n", hostname, len);
        fclose(fp);
        return;
    }

    fclose(fp);
}

int
main(int argc, char **argv)
{
    char *login_argv[] = { LOGIN_PATH, NULL };
    char *start_argv[] = { SHELL_PATH, INIT_RC_PATH, NULL };
    char *envp[] = { NULL };

    /* Initialize the system hostname */
    init_hostname();

    /* Start the init.rc */
    spawn(SHELL_PATH, start_argv, envp, 0);
    start_argv[1] = NULL;

    /* Start the login manager */
    spawn(login_argv[0], login_argv, envp, 0);
    for (;;);
    return 0;
}

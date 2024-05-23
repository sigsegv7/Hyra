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

/* Maintenance shell */

#include <sys/reboot.h>
#include <sys/auxv.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include "mshell.h"

#define INPUT_SIZE 32
#define MAX_FILE_SIZE 1024
#define TTY_DEV "/dev/tty1"
#define PROMPT "mshell> "

struct mshell_state {
    uint8_t running : 1;
    char input[INPUT_SIZE];
};

static void
help(void)
{
    printf(
        "MSHELL COMMANDS\n"
        "\thelp - show this message\n"
        "\treboot - reboot the system\n"
        "\ttty - show the current TTY\n"
        "\tpagesize - get the current page size\n"
        "\tkversion - get the kernel version\n"
        "\tmemstat - get info about memory\n"
        "\texit - exit the shell\n"
    );
}

static void
print_file(const char *path)
{
    char buf[MAX_FILE_SIZE];
    int fd = open(path, O_RDONLY);
    int len;

    if (fd < 0) {
        printf("Failed to open %s\n", path);
        return;
    }

    len = read(fd, buf, sizeof(buf));
    if (len > MAX_FILE_SIZE) {
        len = MAX_FILE_SIZE - 1;
    }

    if (len > 0) {
        buf[len] = '\0';
        printf("%s", buf);
    }

    close(fd);
}

static void
parse_input(struct mshell_state *state)
{
    char cmd[INPUT_SIZE];
    char *p = &state->input[0];
    size_t cmd_idx = 0;

    /* Skip leading whitespace */
    while (*p && *p == ' ')
        ++p;

    /* Push the command */
    while (*p && *p != '\n')
        cmd[cmd_idx++] = *p++;

    cmd[cmd_idx] = '\0';

    /* Ignore empty commands */
    if (cmd_idx == 0)
        return;

    if (strcmp(cmd, "reboot") == 0) {
        reboot(REBOOT_DEFAULT);
    } else if (strcmp(cmd, "pagesize") == 0) {
        printf("%d\n", auxv_entry(AT_PAGESIZE));
    } else if (strcmp(cmd, "tty") == 0) {
        printf("%s\n", TTY_DEV);
    } else if (strcmp(cmd, "help") == 0) {
        help();
    } else if (strcmp(cmd, "exit") == 0) {
        state->running = 0;
    } else if (strcmp(cmd, "kversion") == 0) {
        print_file("/proc/version");
    } else if (strcmp(cmd, "memstat") == 0) {
        print_file("/proc/memstat");
    } else {
        printf("Unknown command '%s'\n", cmd);
        printf("Use 'help' for help\n");
    }
}

int
mshell_enter(void)
{
    int fd, tmp;
    size_t input_idx;
    struct termios tm_old, tm;
    struct mshell_state state = {
        .running = 1
    };

    /* Try opening the TTY */
    fd = open(TTY_DEV, O_RDONLY);
    if (fd < 0)
        return -1;

    /* Set raw mode */
    tcgetattr(fd, &tm_old);
    tm = tm_old;
    tm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fd, 0, &tm);

    printf("%s", PROMPT);
    input_idx = 0;

    while (state.running) {
        if (read(fd, &tmp, 1) <= 0)
            continue;

        switch (tmp) {
        case '\n':
            /*
             * Parse the input, the newline is so that any printed
             * text that is the result of a command can have
             * its own lines.
             */
            printf("\n");
            parse_input(&state);

            /* Reset the buffer */
            input_idx = 0;
            state.input[0] = '\0';

            printf("%s", PROMPT);
            fflush(stdout);
            break;
        case '\b':
            if (input_idx > 0) {
                /*
                 * Replace the last char with a '\0' and rewrite
                 * the prompt.
                 */
                state.input[--input_idx] = '\0';
                printf("\b");
                fflush(stdout);
                continue;
            }

            break;
        default:
            if (input_idx < INPUT_SIZE) {
                state.input[input_idx++] = tmp;
                state.input[input_idx] = '\0';

                printf("%c", tmp);
                fflush(stdout);
            }

            break;
        }

    }

    /* Restore TTY state */
    tcsetattr(fd, 0, &tm_old);
    close(fd);
    return 0;
}

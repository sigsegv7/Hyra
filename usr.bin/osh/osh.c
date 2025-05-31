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
#include <sys/cdefs.h>
#include <sys/reboot.h>
#include <sys/spawn.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#define prcons(FD, STR) write((FD), (STR), strlen((STR)))
#define is_ascii(C) ((C) >= 0 && (C) <= 128)
#define WELCOME \
    ":::::::::::::::::::::::::::::::::::::::\n" \
    ":: OSMORA GATEWAY ~ Every key echos  ::\n" \
    ":: ..... Proceed with purpose .....  ::\n" \
    ":::::::::::::::::::::::::::::::::::::::\n"

#define HELP \
    "Default commands:\n" \
    "help     - Display this help message\n" \
    "echo     - Print the arguments to the console\n" \
    "reboot   - Reboot the machine\n" \
    "shutdown - Power off the machine\n" \
    "kmsg     - Print kernel message buffer\n" \
    "fetch    - System information\n" \
    "exit     - Exit the shell\n"

#define PROMPT "[root::osmora]~ "

static char buf[64];
static uint8_t i;
static int running;

struct command {
    const char *name;
    const char *path;
    void (*func)(int fd, int argc, char *argv[]);
};

void
cmd_help(int fd, int argc, char *argv[])
{
    prcons(fd, HELP);
}

void
cmd_exit(int fd, int argc, char *argv[])
{
    running = 0;
}

void
cmd_reboot(int fd, int argc, char *argv[])
{
    cpu_reboot(REBOOT_RESET);
}

void
cmd_shutdown(int fd, int argc, char *argv[])
{
    cpu_reboot(REBOOT_POWEROFF | REBOOT_HALT);
}

void
cmd_echo(int fd, int argc, char *argv[])
{
    for (i = 1; i < argc; i++) {
        prcons(fd, argv[i]);
        prcons(fd, " ");
    }
    prcons(fd, "\n");
}

int
parse_args(char *input, char *argv[], int max_args)
{
    int argc = 0;

    while (*input != '\0') {
        /* skip leading spaces */
        while (*input == ' ') {
            input++;
        }

        /* check if empty */
        if (*input == '\0') {
            break;
        }

        if (argc < max_args) {
            argv[argc++] = input; /* mark start of the argument */
        }
        /* move forward until next space or end */
        while (*input != '\0' && *input != ' ') {
            input++;
        }

        /* end */
        if (*input != '\0') {
            *input = '\0';
            input++;
        }
    }

    return argc;
}

static char *
getstr(int fd)
{
    char c;
    uint8_t input;
    i = 0;

    for (;;) {
        if (read(fd, &input, 2) <= 0) {
            continue;
        }

        c = input & 0xFF;

        /* return on newline */
        if (c == '\n') {
            buf[i] = '\0';
            write(fd, "\n", 1);
            return buf;
        }

        /* handle backspaces and DEL */
        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
                write(fd, "\b \b", 3);
            }
        } else if (is_ascii(c) && i < sizeof(buf) - 1) {
            /* write to fd and add to buffer */
            buf[i++] = c;
            write(fd, &c, 1);
        }
    }
}

static void
command_run(struct command *cmd, int fd, int argc, char *argv[])
{
    if (cmd->func != NULL) {
        cmd->func(fd, argc, argv);
        return;
    }

    if (cmd->path != NULL) {
        spawn(cmd->path, SPAWN_WAIT);
    }
}

struct command cmds[] = {
    {"help", NULL, cmd_help},
    {"echo", NULL, cmd_echo},
    {"exit", NULL, cmd_exit},
    {"reboot", NULL, cmd_reboot},
    {"shutdown", NULL, cmd_shutdown},
    {"kmsg", "/usr/bin/kmsg", NULL},
    {"fetch", "/usr/bin/fetch", NULL},
    {NULL, NULL}
};

int
main(void)
{
    int fd, found, argc;
    char *input, *argv[16];
    char c;

    if ((fd = open("/dev/console", O_RDWR)) < 0) {
        return fd;
    }

    i = 0;
    running = 1;
    found = 0;

    prcons(fd, WELCOME);
    while (running) {
        prcons(fd, PROMPT);

        input = getstr(fd);
        if (input[0] == '\0') {
            continue;
        }

        argc = parse_args(input, argv, sizeof(argv));
        if (argc == 0) {
            continue;
        }

        for (i = 0; cmds[i].name != NULL; i++) {
            if (strcmp(input, cmds[i].name) == 0) {
                command_run(&cmds[i], fd, argc, argv);
                found = 1;
                break;
            }
        }

        if (found == 0) {
            prcons(fd, "Unrecognized command\n");
        }

        found = 0;
        buf[0] = '\0';
    }
    return 0;
}

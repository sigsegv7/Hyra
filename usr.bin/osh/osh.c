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
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define is_ascii(C) ((C) >= 0 && (C) <= 128)
#define WELCOME \
    ":::::::::::::::::::::::::::::::::::::::\n" \
    ":: OSMORA GATEWAY ~ Every key echos  ::\n" \
    ":: ..... Proceed with purpose .....  ::\n" \
    ":::::::::::::::::::::::::::::::::::::::"

#define HELP \
    "Default commands:\n" \
    "help     - Display this help message\n" \
    "echo     - Print the arguments to the console\n" \
    "reboot   - Reboot the machine\n" \
    "shutdown - Power off the machine\n" \
    "kmsg     - Print kernel message buffer\n" \
    "fetch    - System information\n" \
    "kfg      - Start up kfgwm\n"     \
    "bell     - Toggle backspace bell\n" \
    "time     - Get the current time\n" \
    "clear    - Clear the screen\n"     \
    "exit     - Exit the shell"

#define PROMPT "[root::osmora]~ "

static char buf[64];
static uint8_t i;
static int running;
static int bell_fd;
static bool bs_bell = true; /* Beep on backspace */

struct builtin_cmd {
    const char *name;
    void (*func)(int argc, char *argv[]);
};

static void
cmd_help(int argc, char *argv[])
{
    puts(HELP);
}

static void
cmd_exit(int argc, char *argv[])
{
    running = 0;
}

static void
cmd_reboot(int argc, char *argv[])
{
    cpu_reboot(REBOOT_RESET);
}

static void
cmd_shutdown(int argc, char *argv[])
{
    cpu_reboot(REBOOT_POWEROFF | REBOOT_HALT);
}

static void
cmd_echo(int argc, char *argv[])
{
    for (i = 1; i < argc; i++) {
        fputs(argv[i], stdout);
        putchar(' ');
    }
    putchar('\n');
}

static void
cmd_clear(int argc, char *argv[])
{
    fputs("\033[H", stdout);
}

static void
cmd_bell(int argc, char *argv[])
{
    const char *usage_str = "usage: bell [on/off]";
    const char *arg;

    if (argc < 2) {
        puts(usage_str);
        return;
    }

    arg = argv[1];
    if (strcmp(arg, "on") == 0) {
        bs_bell = true;
    } else if (strcmp(arg, "off") == 0) {
        bs_bell = false;
    } else {
        puts(usage_str);
    }
}

static int
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
getstr(void)
{
    char c;
    int input;
    uint32_t beep_payload;

    i = 0;

    /*
     * Prepare the beep payload @ 500 Hz
     * for 20ms
     */
    beep_payload = 500;
    beep_payload |= (30 << 16);

    for (;;) {
        if ((input = getchar()) < 0) {
            continue;
        }

        c = (char)input;

        /* return on newline */
        if (c == '\n') {
            buf[i] = '\0';
            putchar('\n');
            return buf;
        }

        /* handle backspaces and DEL */
        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
                fputs("\b \b", stdout);
            } else if (bell_fd > 0 && bs_bell) {
                write(bell_fd, &beep_payload, sizeof(beep_payload));
            }
        } else if (is_ascii(c) && i < sizeof(buf) - 1) {
            /* write to fd and add to buffer */
            buf[i++] = c;
            putchar(c);
        }
    }
}

static void
builtin_run(struct builtin_cmd *cmd, int argc, char *argv[])
{
    if (cmd->func != NULL) {
        cmd->func(argc, argv);
        return;
    }
}

static int
cmd_run(const char *input, int argc, char *argv[])
{
    char bin_path[256];
    char *envp[1] = { NULL };
    int error;

    snprintf(bin_path, sizeof(bin_path), "/usr/bin/%s", input);

    /* See if we can access it */
    if (access(bin_path, F_OK) != 0) {
        return -1;
    }

    if ((error = spawn(bin_path, argv, envp, SPAWN_WAIT)) < 0) {
        return error;
    }

    return 0;
}

struct builtin_cmd cmds[] = {
    {"help",cmd_help},
    {"echo",cmd_echo},
    {"exit",cmd_exit},
    {"reboot",cmd_reboot},
    {"shutdown", cmd_shutdown},
    {"bell", cmd_bell},
    {"clear", cmd_clear},
    {NULL, NULL}
};

int
main(void)
{
    int found, argc;
    char *input, *argv[16];
    char c;

    i = 0;
    running = 1;
    found = 0;
    bell_fd = open("/dev/beep", O_WRONLY);

    puts(WELCOME);
    while (running) {
        fputs(PROMPT, stdout);

        input = getstr();
        if (input[0] == '\0') {
            continue;
        }

        argc = parse_args(input, argv, sizeof(argv));
        if (argc == 0) {
            continue;
        }

        for (i = 0; cmds[i].name != NULL; i++) {
            if (strcmp(input, cmds[i].name) == 0) {
                builtin_run(&cmds[i], argc, argv);
                found = 1;
                break;
            }
        }

        if (found == 0) {
            if (cmd_run(input, argc, argv) < 0) {
                puts("Unrecognized command");
            }
        }

        found = 0;
        buf[0] = '\0';
    }
    return 0;
}

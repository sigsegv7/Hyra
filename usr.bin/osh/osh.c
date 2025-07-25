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
#include <sys/errno.h>
#include <sys/spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define is_printable(C) ((C) >= 32 && (C) <= 126)
#define is_ascii(C) ((C) >= 0 && (C) <= 128)

#define COMMENT '@'
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
    "date     - Get the current date\n" \
    "clear    - Clear the screen\n"     \
    "exit     - Exit the shell"

#define PROMPT "[%s::osmora]~ "

static char buf[64];
static int running;
static int bell_fd;
static bool bs_bell = true; /* Beep on backspace */

static void cmd_help(int argc, char *argv[]);
static void cmd_echo(int argc, char *argv[]);
static void cmd_exit(int argc, char *argv[]);
static void cmd_reboot(int argc, char *argv[]);
static void cmd_shutdown(int argc, char *argv[]);
static void cmd_bell(int argc, char *argv[]);
static void cmd_clear(int argc, char *argv[]);

struct builtin_cmd {
    const char *name;
    void (*func)(int argc, char *argv[]);
};

/*
 * Results after parsing a command
 *
 * @bg: Run command in background
 */
struct parse_state {
    uint8_t bg : 1;
};

static struct builtin_cmd cmds[] = {
    {"help",cmd_help},
    {"exit",cmd_exit},
    {"reboot",cmd_reboot},
    {"shutdown", cmd_shutdown},
    {"bell", cmd_bell},
    {"clear", cmd_clear},
    {NULL, NULL}
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
cmd_clear(int argc, char *argv[])
{
    fputs("\033[2J", stdout);
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
parse_args(char *input, char *argv[], int max_args, struct parse_state *p)
{
    int argc = 0;

    /* ignore comments */
    if (*input == '@') {
        return 0;
    }

    /* setup default state */
    p->bg = 0;

    /* parse loop */
    while (*input != '\0') {
        /* skip leading spaces */
        while (*input == ' ') {
            input++;
        }

        /* check if empty */
        if (*input == '\0') {
            break;
        }

        /* comment? */
        if (*input == COMMENT) {
            break;
        }

        /* run in background? */
        if (*input == '&') {
            p->bg = 1;
        }

        if (argc < max_args) {
            argv[argc++] = input; /* mark start of the argument */
        }
        /* move forward until next space or end */
        while (*input != '\0' && *input != ' ') {
            /* ignore comments */
            if (*input == COMMENT) {
                return 0;
            }

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

/*
 * Grab a string from stdin and return
 * the resulting offset within the input
 * buffer we are at.
 */
static uint8_t
getstr(void)
{
    char c;
    int input;
    uint32_t beep_payload;
    uint8_t buf_i = 0;

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
        if (c == '\t') {
            continue;
        }

        /* return on newline */
        if (c == '\n') {
            buf[buf_i] = '\0';
            putchar('\n');
            return buf_i;
        }

        /* handle backspaces and DEL */
        if (c == '\b' || c == 127) {
            if (buf_i > 0) {
                buf_i--;
                fputs("\b \b", stdout);
            } else if (bell_fd > 0 && bs_bell) {
                write(bell_fd, &beep_payload, sizeof(beep_payload));
            }
        } else if (is_printable(c) && buf_i < sizeof(buf) - 1) {
            /* write to fd and add to buffer */
            buf[buf_i++] = c;
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
    char bin_path[512];
    char *envp[1] = { NULL };
    pid_t child;

    /*
     * If we can access the raw input as a file, try to
     * spawn it as a program. This case would run if for
     * example, the user entered /usr/sbin/foo, or some
     * path directly into the console.
     */
    if (access(input, F_OK) == 0) {
        child = spawn(input, argv, envp, 0);
        if (child < 0) {
            return child;
        }
        return child;
    }

    snprintf(bin_path, sizeof(bin_path), "/usr/bin/%s", input);

    /* See if we can access it */
    if (access(bin_path, F_OK) != 0) {
        return -1;
    }

    if ((child = spawn(bin_path, argv, envp, 0)) < 0) {
        return child;
    }

    return child;
}

/*
 * Match a command with a builtin or binary
 *
 * @input: Command input
 * @argc: Argument count
 * @argv: Argument vector
 */
static int
command_match(const char *input, int argc, char *argv[])
{
    int found = 0;
    int i;
    pid_t child = -1;

    for (i = 0; cmds[i].name != NULL; i++) {
        if (strcmp(input, cmds[i].name) == 0) {
            builtin_run(&cmds[i], argc, argv);
            found = 1;
        }
    }

    if (found == 0) {
        if ((child = cmd_run(input, argc, argv)) < 0) {
            puts("Unrecognized command");
            return -1;
        }
    }

    return child;
}

static void
script_skip_comment(int fd)
{
    char c;

    while (c != '\n') {
        if (read(fd, &c, 1) <= 0)
            break;
    }
}

/*
 * Parse a single line typed in from the
 * user.
 *
 * @input: Input line
 */
static int
parse_line(char *input)
{
    int argc;
    char *argv[16];
    struct parse_state state = {0};
    pid_t child;

    /* Ensure the aux vector is zeored */
    memset(argv, 0, sizeof(argv));

    /*
     * Grab args from the user, there should be
     * at least one.
     */
    argc = parse_args(input, argv, sizeof(argv), &state);
    if (argc == 0) {
        return -EAGAIN;
    }

    child = command_match(input, argc, argv);
    if (child > 0 && !state.bg) {
        waitpid(child, NULL, 0);
    }

    return 0;
}

static int
open_script(const char *pathname)
{
    int fd, argc, buf_i = 0;
    char c, *input;
    char buf[256];

    fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        printf("osh: failed to open %s\n", pathname);
        return fd;
    }

    while (read(fd, &c, 1) > 0) {
        /* Skip comments */
        if (c == COMMENT) {
            script_skip_comment(fd);
            continue;
        }

        /* Skip blank newlines */
        if (c == '\n' && buf_i == 0) {
            continue;
        }

        if (buf_i >= sizeof(buf) - 1) {
            buf_i = 0;
        }

        if (c == '\n') {
            buf[buf_i] = '\0';
            parse_line(buf);
            buf_i = 0;
            continue;
        }
        buf[buf_i++] = c;
    }

    return 0;
}

static void
dump_file(const char *pathname)
{
    FILE *file;
    char buf[64];
    int fd;

    file = fopen(pathname, "r");
    if (file == NULL) {
        return;
    }

    while (fgets(buf, sizeof(buf), file) != NULL) {
        printf("%s", buf);
    }

    fclose(file);
}

int
main(int argc, char **argv)
{
    int found, prog_argc;
    int stdout_fd;
    uint8_t buf_i;
    char *p;
    char c;
    pid_t child;

    if (argc > 1) {
        return open_script(argv[1]);
    }

    running = 1;
    bell_fd = open("/dev/beep", O_WRONLY);
    dump_file("/etc/motd");

    while (running) {
        printf(PROMPT, getlogin());

        buf_i = getstr();
        if (buf[0] == '\0') {
            continue;
        }

        buf[buf_i] = '\0';
        if (parse_line(buf) < 0) {
            continue;
        }

        buf[0] = '\0';
    }
    return 0;
}

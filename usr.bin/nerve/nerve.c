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

#include <sys/errno.h>
#include <sys/console.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

/* Verb numeric defs (see string defs) */
#define VERB_UNKNOWN    -1
#define VERB_POKE       0x0000

/* Verb string defs (see numeric defs) */
#define SVERB_POKE "poke"

/* Nerve numeric defs (see string defs) */
#define NERVE_UNKNOWN  -1
#define NERVE_CONSATTR 0x0000
#define NERVE_CONSFEAT 0x0001

/* Nerve string defs (see numeric defs) */
#define SNERVE_CONSATTR "consattr"
#define SNERVE_CONSFEAT "consfeat"

/* Misc defines */
#define NERVE_PACKET_LEN 16

struct verb_handler;

static int poke_nerve(const char *nerve, struct verb_handler *h);

/*
 * Contains verb handlers called when a verb
 * (e.g., 'poke') is matched.
 */
struct verb_handler {
    int(*run)(const char *nerve, struct verb_handler *h);
    char **argv;
    size_t argc;
};

/*
 * Holds information that may be sent down
 * my nerves.
 *
 * Example: nerve poke <x> 1 0 1
 *                         * * *
 *     +--------+         / / /
 *     | meow   | <------+ / /
 *     |--------|         / /
 *     | foo    | <------+ /
 *     |--------|         /
 *     | foobar | <------+
 *     +--------+
 *       packet
 */
struct nerve_payload {
    uint32_t packet[NERVE_PACKET_LEN];
    uint16_t len;
};

/*
 * Verb handler table, when a verb is matched,
 * its respective handler is called.
 */
static struct verb_handler verbtab[] = {
    { poke_nerve }
};

/*
 * Print list of available options as well as
 * information about the program.
 */
static void
help(void)
{
    printf(
        "nerve: usage: nerve <verb> [ .. data ..]\n"
        "verb 'poke': Poke a control (/ctl) nerve\n"
        "???????????????? NERVES ????????????????\n"
        "consattr: Console attributes\n"
        "consfeat: Console features\n"
    );
}

/*
 * The user gets to send data down my nerves through
 * a nerve payload. This function acquires the nerve
 * payload. Please don't hurt me.
 *
 * @argc: Number of arguments within argv
 * @argv: Argument vector
 * @res: Where the payload goes
 */
static int
get_nerve_payload(int argc, char *argv[], struct nerve_payload *res)
{
    char *payload_str;
    uint32_t datum;

    /* Do we have a nerve payload? */
    if (argc < 4) {
        printf("[!] missing nerve payload\n");
        return -1;
    }

    /* Reset fields */
    res->len = 0;
    memset(res->packet, 0, sizeof(res->packet));

    /* Start grabbing bytes */
    for (int i = 3; i < argc; ++i) {
        payload_str = argv[i];
        datum = atoi(payload_str);
        res->packet[res->len++] = datum;
    }

    return 0;
}

/*
 * Poke a control nerve located in /ctl/
 *
 * @nerve: Name of the nerve (e.g., consattr)
 * @h: Verb handler, instance of self
 *
 * Returns less than zero if the nerve does not
 * match.
 */
static int
poke_nerve(const char *nerve, struct verb_handler *h)
{
    struct nerve_payload payload;
    int error, nerve_idx = -1;

    if (nerve == NULL || h == NULL) {
        return -EINVAL;
    }

    /*
     * Now we need to parse the nerve string
     * and see if it matches with anything
     * that we know.
     */
    switch (*nerve) {
    case 'c':
        if (strcmp(nerve, SNERVE_CONSATTR) == 0) {
            nerve_idx = NERVE_CONSATTR;
        } else if (strcmp(nerve, SNERVE_CONSFEAT) == 0) {
            nerve_idx = NERVE_CONSFEAT;
        }

        error = get_nerve_payload(h->argc, h->argv, &payload);
        if (error < 0) {
            printf("[!] nerve error\n");
            return -1;
        }
    }

    switch (nerve_idx) {
    case NERVE_CONSATTR:
        {
            struct console_attr c;
            int fd;

            c.cursor_x = payload.packet[0];
            c.cursor_y = payload.packet[1];

            fd = open("/ctl/console/attr", O_WRONLY);
            write(fd, &c, sizeof(c));
            close(fd);
            break;
        }
    case NERVE_CONSFEAT:
        {
            struct console_feat f;
            int fd;

            f.ansi_esc = payload.packet[0] & 0xFF;
            f.show_curs = payload.packet[1] & 0xFF;

            fd = open("/ctl/console/feat", O_WRONLY);
            write(fd, &f, sizeof(f));
            close(fd);
            break;
        }
    default:
        printf("[&^]: This is not my nerve.\n");
        break;
    }

    return -1;
}

/*
 * Convert a string verb, passed in through the command
 * line, into a numeric definition
 *
 * @verb: String verb
 */
static int
verb_to_def(const char *verb)
{
    if (verb == NULL) {
        return -EINVAL;
    }

    /*
     * Parse the verb and try to match it against
     * a constant.
     *
     * TODO: Add 'peek'
     *
     * XXX: Here we are first matching the first character
     *      before we match the entire verb as that is more
     *      efficient than scanning each entire string until
     *      one matches.
     */
    switch (*verb) {
    case 'p':
        if (strcmp(verb, SVERB_POKE) == 0) {
            return VERB_POKE;
        }
    default:
        printf("[!] bad verb \"%s\"\n", verb);
        return VERB_UNKNOWN;
    }

    return VERB_UNKNOWN;
}

int
main(int argc, char **argv)
{
    struct verb_handler *verbd;
    int verb;

    if (argc < 2) {
        help();
        return -1;
    }

    verb = verb_to_def(argv[1]);
    if (verb < 0) {
        return -1;
    }

    /* Make sure the arguments match */
    switch (verb) {
    case VERB_POKE:
        if (argc < 3) {
            printf("[!] missing nerve name\n");
            help();
            return -1;
        }
        break;
    }

    verbd = &verbtab[verb];
    verbd->argv = argv;
    verbd->argc = argc;
    return verbd->run(argv[2], verbd);
}

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
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

char *optarg;
int optind, opterr, optopt;

int
getopt(int argc, char *argv[], const char *optstring)
{
    size_t optstr_len;
    char *arg;
    bool has_arg = false;

    if (argc == 0 || optstring == NULL) {
        opterr = -EINVAL;
        return -1;
    }

    if (optind >= argc) {
        return -1;
    }

    arg = argv[optind];
    optstr_len = strlen(optstring);

    /* Non option argument? */
    if (arg[0] != '-') {
        return -1;
    }

    /*
     * We will look through each possible flag/option
     * in the optstring and match it against our arg.
     */
    for (size_t i = 0; i < optstr_len; ++i) {
        if (arg[1] != optstring[i]) {
            continue;
        }

        /*
         * If this option has a ':' right next to it,
         * it also has an argument.
         */
        if (i < optstr_len - 1) {
            if (optstring[i + 1] == ':') {
                has_arg = true;
            }
        }

        break;
    }

    /*
     * Handle cases where the option has an argument
     * with it (-opt=arg)
     */
    if (has_arg && optind < argc ) {
        if (arg[2] != '=') {
            opterr = -EINVAL;
            return -1;
        }
        optarg = &arg[3];
        ++optind;
    }

    ++optind;
    return arg[1];
}

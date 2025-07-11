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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "crc32.h"
#include "frame.h"
#include "core.h"

static void
parse_core(const struct core *dump)
{
    uint32_t checksum;

    printf("-- CORE DUMP OF PID %d CRASH -- \n", dump->pid);
    printf("Faulting address: %p\n", dump->fault_addr);
    core_dumpframe(dump);

    checksum = crc32(dump, sizeof(*dump) - sizeof(checksum));
    if (checksum != dump->checksum) {
        printf("!! WARNING: coredump might be corrupt !!\n");
    }
}

int
main(int argc, char **argv)
{
    int fd;
    struct core core;

    if (argc < 2) {
        printf("usage: readcore <coredump>\n");
        return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 2) {
        printf("readcore: Could not open \"%s\"\n", argv[1]);
        return fd;
    }

    if (read(fd, &core, sizeof(core)) < sizeof(core)) {
        printf("readcore: bad read()\n");
        return -1;
    }

    parse_core(&core);
    close(fd);
    return 0;
}

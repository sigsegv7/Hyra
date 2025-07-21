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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <oemu/cpu.h>

static struct oemu_cpu core_0;
struct sysmem g_mem;

static void
help(void)
{
    printf(
        "OSMORA OSMX64 Emulator\n"
        "usage: oemu <binary file>\n"
    );
}

/*
 * Allocate and initialize platform
 * memory.
 */
static int
mem_init(void)
{
    printf("allocating 0x%x bytes of memory\n", MEMORY_SIZE);
    g_mem.mem_size = MEMORY_SIZE;
    g_mem.mem = malloc(MEMORY_SIZE);
    if (g_mem.mem == NULL) {
        printf("failed to allocate memory\n");
        return -1;
    }
}

/*
 * Load a program specified by a path into
 * memory for execution.
 */
static int
program_load(const char *path, paddr_t loadoff)
{
    void *mem = g_mem.mem;
    size_t size;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("failed to open \"%s\"\n", path);
        return -ENOENT;
    }

    /* Grab the size of the file */
    size = lseek(fd, 0, SEEK_END);
    lseek(fd, loadoff, SEEK_SET);
    printf("loading size %d\n", size);

    /* Is it too big? */
    if (size >= g_mem.mem_size) {
        printf("program too big !! (memsize=%x)\n", g_mem.mem_size);
        close(fd);
        return -1;
    }

    printf("read data into %p\n", mem);
    printf("read %d bytes\n", read(fd, mem, size));
    close(fd);
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        help();
        return -1;
    }

    /* Initialize memory */
    if (mem_init() < 0) {
        return -1;
    }

    /* Put the CPU in a known state */
    cpu_reset(&core_0);

    /*
     * Load the program and send the little guy off
     * to start nomming those 32-bit instructions
     */
    if (program_load(argv[1], 0x00000000) < 0) {
        return -1;
    }
    cpu_kick(&core_0, &g_mem);
    free(g_mem.mem);
    return 0;
}

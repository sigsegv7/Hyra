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

#include <sys/sched.h>
#include <sys/vmstat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define MIB_PER_GIB 1024

static void
print_size_mib(const char *name, size_t mib)
{
    if (name == NULL) {
        return;
    }

    if (mib >= MIB_PER_GIB) {
        printf("%s: %d GiB\n", name, mib / MIB_PER_GIB);
    } else {
        printf("%s: %d MiB\n", name, mib);
    }
}

static void
get_vm_stat(void)
{
    struct vm_stat vmstat;
    int retval, fd;

    fd = open("/ctl/vm/stat", O_RDONLY);
    if (fd < 0) {
        printf("failed to open '/ctl/vm/stat'\n");
        return;
    }

    retval = read(fd, &vmstat, sizeof(vmstat));
    if (retval <= 0) {
        printf("failed to read vmstat\n");
        return;
    }

    close(fd);
    print_size_mib("memory available", vmstat.mem_avail);
    print_size_mib("memory used", vmstat.mem_used);
}

static void
get_sched_stat(void)
{
    struct sched_stat stat;
    struct sched_cpu *cpu;
    double nonline, noffline;
    uint16_t online_percent;
    int fd;

    fd = open("/ctl/sched/stat", O_RDONLY);
    if (fd < 0) {
        printf("failed to get sched stat\n");
        return;
    }
    if (read(fd, &stat, sizeof(stat)) < 0) {
        printf("failed to read sched stat\n");
        return;
    }

    close(fd);
    noffline = stat.nhlt;
    nonline = (stat.ncpu - noffline);
    online_percent = (uint16_t)(((double)nonline / (nonline + noffline)) * 100);

    printf("-------------------------------\n");
    printf("Number of tasks: %d\n", stat.nproc);
    printf("Number of cores online: %d\n", stat.ncpu);
    printf("Scheduler quantum: %d usec\n", stat.quantum_usec);
    printf("CPU is %d%% online\n", online_percent);
    printf("-------------------------------\n");

    /*
     * Log out some per-cpu information
     */
    for (int i = 0; i < stat.ncpu; ++i) {
        cpu = &stat.cpus[i];
        printf("[cpu %d]: %d switches\n", i, cpu->nswitch);
    }
}

int
main(void)
{
    get_sched_stat();
    get_vm_stat();
    return 0;
}

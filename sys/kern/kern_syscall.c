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

#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/krq.h>

scret_t(*g_sctab[])(struct syscall_args *) = {
    NULL,       /* SYS_none */
    sys_exit,   /* SYS_exit */
    sys_open,   /* SYS_open */
    sys_read,   /* SYS_read */
    sys_close,  /* SYS_close */
    sys_stat,   /* SYS_stat */
    sys_sysctl, /* SYS_sysctl */
    sys_write,  /* SYS_write */
    sys_spawn,  /* SYS_spawn */
    sys_reboot, /* SYS_reboot */
    sys_mmap,   /* SYS_mmap */
    sys_munmap, /* SYS_munap */
    sys_access, /* SYS_access */
    sys_lseek,  /* SYS_lseek */
    sys_sleep,  /* SYS_sleep */
    sys_inject, /* SYS_inject */
    sys_getpid, /* SYS_getpid */
    sys_getppid /* SYS_getppid */
};

const size_t MAX_SYSCALLS = NELEM(g_sctab);

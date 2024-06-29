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

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/sched.h>
#include <vm/dynalloc.h>
#include <string.h>

static size_t nthreads = 0;

/*
 * Fork1 - fork and direct a thread to 'ip'
 *
 * @cur: Current process.
 * @flags: Flags to set.
 * @ip: Location for new thread to start at.
 * @newprocp: Will contain new thread if not NULL.
 */
int
fork1(struct proc *cur, int flags, void(*ip)(void), struct proc **newprocp)
{
    struct proc *newproc;
    int status = 0;

    newproc = dynalloc(sizeof(*newproc));
    if (newproc == NULL)
        return -ENOMEM;

    status = md_fork(newproc, cur, (uintptr_t)ip);
    if (status != 0)
        goto done;

    /* Set proc output if we can */
    if (newprocp != NULL)
        *newprocp = newproc;

    newproc->pid = nthreads++;
    sched_enqueue_td(newproc);
done:
    if (status != 0)
        dynfree(newproc);

    return status;
}
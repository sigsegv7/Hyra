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

#include <sys/spawn.h>
#include <sys/wait.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/mman.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/atomic.h>
#include <sys/syslog.h>
#include <sys/syscall.h>
#include <sys/atomic.h>
#include <sys/signal.h>
#include <sys/limits.h>
#include <sys/sched.h>
#include <vm/dynalloc.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("spawn: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define ARGVP_MAX (ARG_MAX / sizeof(void *))

static size_t next_pid = 1;
extern volatile size_t g_nthreads;

/*
 * TODO: envp
 */
struct spawn_args {
    char path[PATH_MAX];
    char argv_blk[ARG_MAX];
    char *argv[ARGVP_MAX];
};

static inline void
try_free_data(void *p)
{
    if (p != NULL) {
        dynfree(p);
    }
}

static void
spawn_thunk(void)
{
    const char *path;
    char pathbuf[PATH_MAX];
    struct proc *cur;
    struct execve_args execve_args;
    struct spawn_args *args;
    char *envp[] = { NULL };

    cur = this_td();
    args = cur->data;
    path = args->path;
    memset(pathbuf, 0, sizeof(pathbuf));
    memcpy(pathbuf, path, strlen(path));

    execve_args.pathname = pathbuf;
    execve_args.argv = (char **)&args->argv[0];
    execve_args.envp = envp;
    path = NULL;

    if (execve(cur, &execve_args) != 0) {
        pr_error("execve failed, aborting\n");
        exit1(this_td(), 0);
    }
    __builtin_unreachable();
}

pid_t
waitpid(pid_t pid, int *wstatus, int options)
{
    struct proc *child, *td;
    pid_t ret;

    td = this_td();
    child = get_child(td, pid);

    if (child == NULL) {
        return -1;
    }

    /* Wait for it to be done */
    while (!ISSET(child->flags, PROC_ZOMB)) {
        sched_yield();
    }


    /* Give back the status */
    if (wstatus != NULL) {
        copyout(&child->exit_status, wstatus, sizeof(*wstatus));
    }

    ret = child->pid;
    proc_reap(child);
    return ret;
}

/*
 * Spawn a new process
 *
 * @cur: Parent (current) process.
 * @func: Address of start code.
 * @p: Data to pass to new process (used for user procs)
 * @flags: Spawn flags.
 * @newprocp: If not NULL, will contain the new process.
 *
 * Returns the PID of the child on success, otherwise an
 * errno value that is less than zero.
 *
 * XXX: `p` is only used by sys_spawn and should be set
 *      to NULL if called in the kernel.
 */
pid_t
spawn(struct proc *cur, void(*func)(void), void *p, int flags, struct proc **newprocp)
{
    struct proc *newproc;
    int error;
    pid_t pid;

    newproc = dynalloc(sizeof(*newproc));
    if (newproc == NULL) {
        pr_error("could not alloc proc (-ENOMEM)\n");
        try_free_data(p);
        return -ENOMEM;
    }

    memset(newproc, 0, sizeof(*newproc));
    error = md_spawn(newproc, cur, (uintptr_t)func);
    if (error < 0) {
        dynfree(newproc);
        try_free_data(p);
        pr_error("error initializing proc\n");
        return error;
    }

    /* Set proc output if we can */
    if (newprocp != NULL) {
        *newprocp = newproc;
    }

    if (!ISSET(cur->flags, PROC_LEAFQ)) {
        TAILQ_INIT(&cur->leafq);
        cur->flags |= PROC_LEAFQ;
    }

    error = proc_init(newproc, cur);
    if (error < 0) {
        dynfree(newproc);
        try_free_data(p);
        pr_error("error initializing proc\n");
        return error;
    }

    newproc->data = p;
    atomic_inc_64(&g_nthreads);

    newproc->pid = next_pid++;
    sched_enqueue_td(newproc);
    pid = newproc->pid;
    return pid;
}

/*
 * Get the child of a process by PID.
 *
 * @cur: Parent process.
 * @pid: Child PID.
 *
 * Returns NULL if no child was found.
 */
struct proc *
get_child(struct proc *cur, pid_t pid)
{
    struct proc *procp;

    TAILQ_FOREACH(procp, &cur->leafq, leaf_link) {
        if (procp == NULL) {
            continue;
        }
        if (procp->pid == pid) {
            return procp;
        }
    }

    return NULL;
}

/*
 * arg0: PID
 * arg1: wstatus
 * arg2: options
 *
 * Returns PID of terminated child, returns
 * -1 on failure.
 */
scret_t
sys_waitpid(struct syscall_args *scargs)
{
    pid_t pid;
    int *u_wstatus;
    int options;

    pid = scargs->arg0;
    u_wstatus = (void *)scargs->arg1;
    options = scargs->arg2;
    return waitpid(pid, u_wstatus, options);
}

/*
 * arg0: The file /path/to/executable
 * arg1: Argv
 * arg2: Envp (TODO)
 * arg3: Optional flags (`flags')
 */
scret_t
sys_spawn(struct syscall_args *scargs)
{
    struct spawn_args *args;
    char *path;
    const char *u_path, **u_argv;
    const char *u_p = NULL;
    struct proc *td;
    int flags, error;
    size_t len, bytes_copied = 0;
    size_t argv_i = 0;

    td = this_td();
    u_path = (const char *)scargs->arg0;
    u_argv = (const char **)scargs->arg1;
    flags = scargs->arg3;

    args = dynalloc(sizeof(*args));
    if (args == NULL) {
        return -ENOMEM;
    }

    error = copyinstr(u_path, args->path, sizeof(args->path));
    if (error < 0) {
        dynfree(args);
        return error;
    }

    memset(args->argv, 0, ARG_MAX);
    for (size_t i = 0; i < ARG_MAX - 1; ++i) {
        error = copyin(&u_argv[argv_i], &u_p, sizeof(u_p));
        if (error < 0) {
            dynfree(args);
            return error;
        }
        if (u_p == NULL) {
            args->argv[argv_i++] = NULL;
            break;
        }

        path = &args->argv_blk[i];
        error = copyinstr(u_p, path, ARG_MAX - bytes_copied);
        if (error < 0) {
            dynfree(args);
            return error;
        }

        args->argv[argv_i++] = &args->argv_blk[i];
        len = strlen(path);
        bytes_copied += (len + 1);
        i += len;
    }

    return spawn(td, spawn_thunk, args, flags, NULL);
}

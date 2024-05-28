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

#include <sys/sched.h>
#include <sys/system.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/exec.h>
#include <sys/filedesc.h>
#include <sys/signal.h>
#include <sys/loader.h>
#include <sys/cdefs.h>
#include <vm/dynalloc.h>
#include <sys/syslog.h>
#include <string.h>

__MODULE_NAME("exec");
__KERNEL_META("$Hyra$: kern_exec.c, Ian Marco Moffett, "
              "exec() implementation");

#define ARG_MAX 1024

/*
 * Allocates a buffer in in `res' and fetches
 * the args from `argv'
 *
 * @argv: User argv
 * @res: Exec args result.
 *
 * XXX: res->argp is dynamically allocated, remember
 *      to free it!
 */
static int
exec_get_args(char **argv, struct exec_args *res)
{
    static char *dmmy_envp[] = {NULL};
    const size_t ARG_LEN = sizeof(char) * ARG_MAX;
    char *argp = NULL;
    void *tmp;

    struct proc *td;
    size_t argp_len = 0;

    if (res == NULL)
        return -EINVAL;

    /* Allocate argp */
    res->argp = dynalloc(ARG_LEN);
    if (res->argp == NULL)
        return -ENOMEM;

    td = this_td();
    res->vas = td->addrsp;
    res->envp = dmmy_envp;

    /* Read argv */
    copyin((uintptr_t)argv, &argp, sizeof(char *));
    for (;;) {
        if (argp == NULL) {
            res->argp[argp_len] = NULL;
            break;
        }

        /* Fetch this arg and get next argp */
        copyinstr((uintptr_t)argp, res->argp[argp_len++], ARG_MAX);
        copyin((uintptr_t)++argv, &argp, sizeof(char *));

        /* Try to resize the argp buffer */
        tmp = dynrealloc(res->argp, ARG_LEN * (argp_len + 1));
        if (tmp == NULL) {
            dynfree(res->argp);
            return -ENOMEM;
        }
        res->argp = tmp;
    }

    return 0;
}

/*
 * Reset the stack of the process.
 *
 * @td: Target thread.
 * @args: Exec args.
 *
 * Returns the new stack pointer.
 */
static uintptr_t
exec_set_stack(struct proc *td, struct exec_args args)
{
    struct vm_range *stack_range;
    uintptr_t stack_top, sp;

    stack_range = &td->addr_range[ADDR_RANGE_STACK];
    stack_top = stack_range->start + (PROC_STACK_SIZE);

    sp = loader_init_stack((void *)stack_top, args);
    return sp;
}

/*
 * execv() implementation.
 *
 * @pathname: Path of file to execute.
 * @argv: Args.
 * @sp_res: Pointer to new stack pointer
 */
static int
execv(char *pathname, char **argv, uintptr_t *sp_res)
{
    char *bin = NULL;
    struct filedesc *filedes;
    struct vm_range *exec_range;
    struct exec_args args;

    struct proc *td = this_td();
    int fd, ret = 0;
    int status;
    size_t bin_size;

    if ((status = exec_get_args(argv, &args)) != 0)
        return status;

    spinlock_acquire(&td->lock);
    fd = open(pathname, O_RDONLY);

    if (fd < 0) {
        ret = -ENOENT;
        goto done;
    }

    filedes = fd_from_fdnum(td, fd);
    if (__unlikely(filedes == NULL)) {
        /*
         * Should not happen. The kernel might be in some
         * erroneous state.
         */
        ret = -EIO;
        goto done;
    }

    lseek(fd, 0, SEEK_END);
    bin_size = filedes->offset;
    lseek(fd, 0, SEEK_SET);

    /* Allocate memory for the binary */
    bin = dynalloc(bin_size);
    if (bin == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Read-in the binary */
    if ((status = read(fd, bin, bin_size)) < 0) {
        ret = status;
        goto done;
    }

    /*
     * Unload the current process image. After we do this,
     * we cannot return in this state until we replace it.
     *
     * XXX: This is one of the last things we do in case of
     *      errors.
     */
    exec_range = &td->addr_range[ADDR_RANGE_EXEC];
    loader_unload(td->addrsp, exec_range);

    /*
     * Now we try to load the new program and hope everything
     * works... If something goes wrong here then we'll be forced
     * to send a SIGSEGV to the thread.
     */
    status = loader_load(td->addrsp, bin, &args.auxv, 0, NULL, exec_range);
    if (status != 0) {
        /* Well shit */
        kprintf("exec: Failed to load new process image\n");
        signal_raise(td, SIGSEGV);
        for (;;);
    }

    *sp_res = exec_set_stack(td, args);
    set_frame_ip(td->tf, args.auxv.at_entry);
done:
    /* We are done, cleanup and release the thread */
    if (bin != NULL) dynfree(bin);
    fd_close_fdnum(td, fd);
    dynfree(args.argp);
    spinlock_release(&td->lock);
    return ret;
}

/*
 * Arg0: Pathname
 * Arg1: Argv
 */
uint64_t
sys_execv(struct syscall_args *args)
{
    uintptr_t sp;
    char pathname[PATH_MAX];
    char **argv = (char **)args->arg1;

    int status;
    struct proc *td = this_td();

    copyinstr(args->arg0, pathname, PATH_MAX);
    if ((status = execv(pathname, argv, &sp)) != 0)
        return status;

    args->ip = get_frame_ip(td->tf);
    args->sp = sp;
    return 0;
}

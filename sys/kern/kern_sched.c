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
#include <sys/schedvar.h>
#include <sys/sched_state.h>
#include <sys/types.h>
#include <sys/timer.h>
#include <sys/cdefs.h>
#include <sys/spinlock.h>
#include <sys/loader.h>
#include <sys/panic.h>
#include <sys/machdep.h>
#include <sys/filedesc.h>
#include <fs/initramfs.h>
#include <vm/dynalloc.h>
#include <vm/physseg.h>
#include <vm/pmap.h>
#include <vm/map.h>
#include <vm/vm.h>
#include <assert.h>
#include <string.h>

#define STACK_PAGES 8
#define STACK_SIZE (STACK_PAGES*vm_get_page_size())

/*
 * The PHYS_TO_VIRT/VIRT_TO_PHYS macros convert
 * addresses to lower and higher half addresses.
 * Userspace addresses are on the lower half,
 * therefore, we can just wrap over these to
 * keep things simple.
 *
 * XXX: TODO: This won't work when not identity mapping
 *            lowerhalf addresses. Once that is updated,
 *            get rid of this.
 */
#define USER_TO_KERN(user) PHYS_TO_VIRT(user)
#define KERN_TO_USER(kern) VIRT_TO_PHYS(kern)

/*
 * Thread ready queue - all threads ready to be
 * scheduled should be added to this queue.
 */
static TAILQ_HEAD(, proc) td_queue;
static size_t nthread = 0;

/*
 * Thread queue lock - all operations to `td_queue'
 * must be done with this lock acquired.
 */
static struct spinlock tdq_lock = {0};

static inline void
sched_oneshot(void)
{
    struct timer timer;
    tmrr_status_t tmr_status;

    tmr_status = req_timer(TIMER_SCHED, &timer);
    __assert(tmr_status == TMRR_SUCCESS);

    timer.oneshot_us(DEFAULT_TIMESLICE_USEC);
}

/*
 * Push a thread into the thread ready queue
 * allowing it to be eventually dequeued
 * and ran.
 */
static void
sched_enqueue_td(struct proc *td)
{
    /* Sanity check */
    if (td == NULL)
        return;

    spinlock_acquire(&tdq_lock);

    td->pid = nthread++;
    TAILQ_INSERT_TAIL(&td_queue, td, link);

    spinlock_release(&tdq_lock);
}

/*
 * Dequeue the first thread in the thread ready
 * queue.
 */
static struct proc *
sched_dequeue_td(void)
{
    struct proc *td = NULL;

    spinlock_acquire(&tdq_lock);

    if (!TAILQ_EMPTY(&td_queue)) {
        td = TAILQ_FIRST(&td_queue);
        TAILQ_REMOVE(&td_queue, td, link);
    }

    spinlock_release(&tdq_lock);
    return td;
}


/*
 * Processor awaiting tasks to be assigned will be here spinning.
 */
__noreturn static void
sched_enter(void)
{
    sched_oneshot();
    for (;;) {
        hint_spinwait();
    }
}

static uintptr_t
sched_init_stack(void *stack_top, char *argvp[], char *envp[], struct auxval auxv)
{
    uintptr_t *sp = stack_top;
    void *env_ptr = NULL, *argv_ptr = NULL;
    size_t argc, envc, len;

    /* Copy argument and environment strings */
    for (envc = 0; envp[envc] != NULL; ++envc) {
        len = strlen(envp[envc]);
        sp -= len - 1;
        memcpy(sp, envp[envc], len);
    }

    __assert(envc >= 1);
    env_ptr = sp;

    for (argc = 0; argvp[argc] != NULL; ++argc) {
        len = strlen(argvp[argc]);
        sp -= len - 1;
        memcpy(sp, argvp[argc], len);
    }

    __assert(argc >= 1);
    argv_ptr = sp;

    /* Ensure the stack is aligned */
    sp = (void *)__ALIGN_DOWN((uintptr_t)sp, 16);
    if (((argc + envc + 1) & 1) != 0)
        --sp;

    AUXVAL(sp, AT_NULL, 0x0);
    AUXVAL(sp, AT_SECURE, 0x0);
    AUXVAL(sp, AT_ENTRY, auxv.at_entry);
    AUXVAL(sp, AT_PHDR, auxv.at_phdr);
    AUXVAL(sp, AT_PHNUM, auxv.at_phnum);
    STACK_PUSH(sp, 0);

    /* Push environment string pointers */
    for (int i = 0; i < envc; ++i) {
        len = strlen(env_ptr);
        sp -= len;

        *sp = (uintptr_t)KERN_TO_USER((uintptr_t)env_ptr);
        env_ptr = (char *)env_ptr + len;
    }

    /* Push argument string pointers */
    STACK_PUSH(sp, 0);
    for (int i = 0; i < argc; ++i) {
        len = strlen(argv_ptr);
        sp -= len;

        *sp = (uintptr_t)KERN_TO_USER((uintptr_t)argv_ptr);
        argv_ptr = (char *)argv_ptr + len;
    }

    STACK_PUSH(sp, argc);

    return (uintptr_t)sp;
}

static uintptr_t
sched_create_stack(struct vas vas, bool user, char *argvp[],
                   char *envp[], struct auxval auxv, struct proc *td)
{
    int status;
    uintptr_t stack;
    const vm_prot_t USER_STACK_PROT = PROT_WRITE | PROT_USER;
    struct vm_range *stack_range = &td->addr_range[ADDR_RANGE_STACK];

    if (!user) {
        stack = (uintptr_t)dynalloc(STACK_SIZE);
        stack_range->start = (uintptr_t)stack;
        stack_range->end = (uintptr_t)stack + STACK_SIZE;
        return sched_init_stack((void *)(stack + STACK_SIZE), argvp, envp, auxv);
    }

    stack = vm_alloc_pageframe(STACK_PAGES);

    stack_range->start = stack;
    stack_range->end = stack + STACK_SIZE;
    status = vm_map_create(vas, stack, stack, USER_STACK_PROT, STACK_SIZE);

    if (status != 0) {
        return 0;
    }

    memset(USER_TO_KERN(stack), 0, STACK_SIZE);
    stack = sched_init_stack((void *)USER_TO_KERN(stack + STACK_SIZE), argvp, envp, auxv);
    return stack;
}

static struct proc *
sched_create_td(uintptr_t rip, char *argvp[], char *envp[], struct auxval auxv,
                struct vas vas, bool is_user, struct vm_range *prog_range)
{
    struct proc *td;
    struct vm_range *exec_range;
    uintptr_t stack;
    struct trapframe *tf;

    tf = dynalloc(sizeof(struct trapframe));
    if (tf == NULL) {
        return NULL;
    }

    td = dynalloc(sizeof(struct proc));
    if (td == NULL) {
        /* TODO: Free stack */
        dynfree(tf);
        return NULL;
    }


    memset(td, 0, sizeof(struct proc));
    stack = sched_create_stack(vas, is_user, argvp, envp, auxv, td);
    if (stack == 0) {
        dynfree(tf);
        dynfree(td);
        return NULL;
    }

    memset(tf, 0, sizeof(struct trapframe));

    /* Setup process itself */
    exec_range = &td->addr_range[ADDR_RANGE_EXEC];
    td->pid = 0;        /* Don't assign PID until enqueued */
    td->cpu = NULL;     /* Not yet assigned a core */
    td->tf = tf;
    td->addrsp = vas;
    td->is_user = is_user;
    if (prog_range != NULL) {
        memcpy(exec_range, prog_range, sizeof(struct vm_range));
    }
    processor_init_pcb(td);

    /* Allocate standard file descriptors */
    __assert(fd_alloc(td) != NULL);       /* STDIN */
    __assert(fd_alloc(td) != NULL);       /* STDOUT */
    __assert(fd_alloc(td) != NULL);       /* STDERR */

    /* Setup trapframe */
    if (!is_user) {
        init_frame(tf, rip, (uintptr_t)stack);
    } else {
        init_frame_user(tf, rip, KERN_TO_USER(stack));
    }
    return td;
}

static void
sched_destroy_td(struct proc *td)
{
    const struct vm_range *stack_range = &td->addr_range[ADDR_RANGE_STACK];

    processor_free_pcb(td);

    /*
     * User stacks are allocated with vm_alloc_pageframe(),
     * while kernel stacks are allocated with dynalloc().
     * We want to check if we are a user program or kernel
     * program to perform the proper deallocation method.
     */
    if (td->is_user) {
        vm_free_pageframe(stack_range->start, STACK_PAGES);
    } else {
        dynfree((void *)stack_range->start);
    }

    /* Close all of the file descriptors */
    for (size_t i = 0; i < PROC_MAX_FDS; ++i) {
        fd_close_fdnum(td, i);
    }

    pmap_free_vas(vm_get_ctx(), td->addrsp);
    dynfree(td);
}

void
sched_exit(void)
{
    struct proc *td;
    struct vas kvas = vm_get_kvas();

    intr_mask();

    td = this_td();

    /* Switch back to the kernel address space and destroy ourself */
    pmap_switch_vas(vm_get_ctx(), kvas);
    sched_destroy_td(td);

    intr_unmask();
    sched_enter();
}

/*
 * Get the current running thread.
 */
struct proc *
this_td(void)
{
    struct sched_state *state;
    struct cpu_info *ci;

    ci = this_cpu();
    state = &ci->sched_state;
    return state->td;
}

/*
 * Thread context switch routine
 */
void
sched_context_switch(struct trapframe *tf)
{
    struct cpu_info *ci = this_cpu();
    struct sched_state *state = &ci->sched_state;
    struct proc *td, *next_td;

    /*
     * If we have no threads, we should not
     * preempt at all.
     */
    if (nthread == 0 || (next_td = sched_dequeue_td()) == NULL) {
        sched_oneshot();
        return;
    }

    /*
     * If we have a thread currently running and we are switching
     * to another, we shall save our current register state
     * by copying the trapframe.
     */
    if (state->td != NULL) {
        td = state->td;
        memcpy(td->tf, tf, sizeof(struct trapframe));
    }

    /* Copy over the next thread's register state to us */
    memcpy(tf, next_td->tf, sizeof(struct trapframe));

    td = state->td;
    state->td = next_td;

    /* Re-enqueue the previous thread if it exists */
    if (td != NULL) {
        sched_enqueue_td(td);
    }

    /* Do architecture specific context switch logic */
    processor_switch_to(td, next_td);

    /* Done, switch out our vas and oneshot */
    pmap_switch_vas(vm_get_ctx(), next_td->addrsp);
    sched_oneshot();
}

void
sched_init(void)
{
    struct proc *init;
    struct vm_range init_range;
    struct auxval auxv = {0}, ld_auxv = {0};
    struct vas vas = pmap_create_vas(vm_get_ctx());
    const char *init_bin, *ld_bin;

    char *ld_path;
    char *argv[] = {"/usr/sbin/init", NULL};
    char *envp[] = {"", NULL};

    TAILQ_INIT(&td_queue);

    if ((init_bin = initramfs_open("/usr/sbin/init")) == NULL) {
        panic("Could not open /usr/boot/init\n");
    }
    if (loader_load(vas, init_bin, &auxv, 0, &ld_path, &init_range) != 0) {
        panic("Could not load init\n");
    }
    if ((ld_bin = initramfs_open(ld_path)) == NULL) {
        panic("Could not open %s\n", ld_path);
    }
    if (loader_load(vas, ld_bin, &ld_auxv, 0x00, NULL, &init_range) != 0) {
        panic("Could not load %s\n", ld_path);
    }

    init = sched_create_td((uintptr_t)ld_auxv.at_entry, argv, envp,
                           ld_auxv, vas, true, &init_range);
    if (init == NULL) {
        panic("Failed to create thread for init\n");
    }

    sched_enqueue_td(init);
}

/*
 * Setup scheduler related things and enqueue AP.
 */
void
sched_init_processor(struct cpu_info *ci)
{
    struct sched_state *sched_state = &ci->sched_state;
    (void)sched_state;      /* TODO */

    sched_enter();

    __builtin_unreachable();
}

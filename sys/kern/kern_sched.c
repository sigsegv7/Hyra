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
#include <sys/machdep.h>
#include <sys/loader.h>
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <sys/filedesc.h>
#include <sys/timer.h>
#include <sys/panic.h>
#include <sys/signal.h>
#include <fs/initramfs.h>
#include <vm/dynalloc.h>
#include <vm/physseg.h>
#include <vm/map.h>
#include <vm/pmap.h>
#include <string.h>
#include <assert.h>

/*
 * Thread ready queues - all threads ready to be
 * scheduled should be added to the toplevel queue.
 */
static struct sched_queue qlist[SCHED_NQUEUE];

/*
 * Global scheduler state.
 */
static size_t nthread = 0;
static schedpolicy_t policy = SCHED_POLICY_MLFQ;

/*
 * Thread queue lock - all operations to `qlist'
 * must be done with this lock acquired.
 *
 * This lock is aligned on a cache line boundary to ensure
 * it has its own cache line to reduce contention. This is
 * because it is constantly acquired and released on every
 * processor.
 */
__cacheline_aligned
static struct spinlock tdq_lock = {0};

/*
 * Lower thread priority.
 */
static inline void
td_pri_lower(struct proc *td)
{
    if (td->priority < (SCHED_NQUEUE - 1))
        ++td->priority;
}

/*
 * Raise thread priority.
 */
static inline void
td_pri_raise(struct proc *td)
{
    if (td->priority > 0)
        --td->priority;
}

/*
 * Called during preemption. We raise the priority
 * if the thread has been rested. If the thread has not
 * been rested, we lower its priority.
 */
static void
td_pri_update(struct proc *td)
{
    if (td->rested) {
        td->rested = 0;
        td_pri_raise(td);
        return;
    }

    td_pri_lower(td);
}

/*
 * Enqueue a thread into the scheduler.
 */
static void
sched_enqueue_td(struct proc *td)
{
    struct sched_queue *queue;

    spinlock_acquire(&tdq_lock);
    queue = &qlist[td->priority];

    TAILQ_INSERT_TAIL(&queue->q, td, link);
    spinlock_release(&tdq_lock);
}

/*
 * Dequeue a thread from a queue.
 */
static struct proc *
sched_dequeue_td(void)
{
    struct sched_queue *queue;
    struct proc *td = NULL;

    spinlock_acquire(&tdq_lock);

    /*
     * Try to pop a thread from a queue. We start at the
     * highest priority which is 0.
     */
    for (size_t i = 0; i < SCHED_NQUEUE; ++i) {
        queue = &qlist[i];

        if (!TAILQ_EMPTY(&queue->q)) {
            td = TAILQ_FIRST(&queue->q);
            TAILQ_REMOVE(&queue->q, td, link);
            break;
        }
    }

    spinlock_release(&tdq_lock);
    return td;
}

/*
 * Create a new thread stack.
 * sched_new_td() helper.
 */
static uintptr_t
sched_create_stack(bool is_user, struct exec_args args, struct proc *td)
{
    int status;
    uintptr_t stack;
    struct vm_range *stack_range;
    const vm_prot_t USERSTACK_PROT = PROT_WRITE | PROT_USER;

    stack_range = &td->addr_range[ADDR_RANGE_STACK];

    /*
     * Kernel stacks can be allocated with dynalloc() as they
     * are on the higher half.
     */
    if (!is_user) {
        stack = (uintptr_t)dynalloc(PROC_STACK_SIZE);
        stack_range->start = stack;
        stack_range->end = stack + PROC_STACK_SIZE;
        return loader_init_stack((void *)(stack + PROC_STACK_SIZE), args);
    }

    stack = vm_alloc_pageframe(PROC_STACK_PAGES);
    if (stack == 0) {
        return 0;
    }

    status = vm_map_create(args.vas, stack, stack, USERSTACK_PROT, PROC_STACK_SIZE);
    if (status != 0) {
        vm_free_pageframe(stack, PROC_STACK_PAGES);
        return 0;
    }

    stack_range->start = stack;
    stack_range->end = stack + PROC_STACK_SIZE;

    memset(USER_TO_KERN(stack), 0, PROC_STACK_SIZE);
    stack = loader_init_stack((void *)USER_TO_KERN(stack + PROC_STACK_SIZE), args);
    return stack;
}

/*
 * Create a new thread.
 *
 * @ip: Instruction pointer to start at.
 * @is_user: True for user program.
 * @exec_args: Common exec args.
 */
static int
sched_new_td(uintptr_t ip, bool is_user, struct exec_args args, struct vm_range *prog_range,
             struct proc **res)
{
    struct vm_range *exec_range;
    struct proc *td;
    struct trapframe *tf;
    uintptr_t stack;
    int retval = 0;

    td = dynalloc(sizeof(struct proc));
    tf = dynalloc(sizeof(struct trapframe));
    if (td == NULL || tf == NULL) {
        retval = -ENOMEM;
        goto done;
    }

    /* Keep them in a known state */
    memset(td, 0, sizeof(*td));
    memset(tf, 0, sizeof(*tf));

    /* Try to create a stack */
    stack = sched_create_stack(is_user, args, td);
    if (stack == 0) {
        retval = -ENOMEM;
        goto done;
    }

    /* Setup initial thread state */
    td->pid = ++nthread;
    td->tf = tf;
    td->addrsp = args.vas;
    td->is_user = is_user;
    processor_init_pcb(td);

    /* Setup each mapping table */
    for (size_t i = 0; i < MTAB_ENTRIES; ++i) {
        TAILQ_INIT(&td->mapspace.mtab[i]);
    }

    /* Setup standard file descriptors */
    __assert(fd_alloc(td, NULL) == 0);  /* STDIN */
    __assert(fd_alloc(td, NULL) == 0);  /* STDOUT */
    __assert(fd_alloc(td, NULL) == 0);  /* STDERR */

    exec_range = &td->addr_range[ADDR_RANGE_EXEC];
    memcpy(exec_range, prog_range, sizeof(struct vm_range));

    /* Init the trapframe */
    if (!is_user) {
        init_frame(tf, ip, stack);
    } else {
        init_frame_user(tf, ip, KERN_TO_USER(stack));
    }
done:
    if (retval != 0 && td != NULL)
        dynfree(td);
    if (retval != 0 && tf != NULL)
        dynfree(td);
    if (retval == 0 && res != NULL)
        *res = td;

    return retval;
}

/*
 * Perform timer oneshot
 *
 * @now: True for shortest timeslice.
 */
static inline void
sched_oneshot(bool now)
{
    struct timer timer;
    size_t usec = (now) ? SHORT_TIMESLICE_USEC : DEFAULT_TIMESLICE_USEC;
    tmrr_status_t tmr_status;

    tmr_status = req_timer(TIMER_SCHED, &timer);
    __assert(tmr_status == TMRR_SUCCESS);

    timer.oneshot_us(usec);
}

/*
 * Enter the schedulera and wait until
 * preemption.
 */
void
sched_enter(void)
{
    sched_oneshot(false);

    for (;;) {
        hint_spinwait();
    }
}

/*
 * Initialize all of the queues.
 */
static void
sched_init_queues(void)
{
    for (size_t i = 0; i < SCHED_NQUEUE; ++i) {
        TAILQ_INIT(&qlist[i].q);
    }
}

/*
 * Load the first thread (init)
 */
static void
sched_load_init(void)
{
    struct exec_args args;
    struct proc *init;
    struct auxval *auxvp = &args.auxv;
    struct vm_range init_range;
    int tmp;

    char *argv[] = {"/usr/sbin/init", NULL};
    char *envp[] = {NULL};
    const char *init_bin;

    if ((init_bin = initramfs_open("/usr/sbin/init")) == NULL)
        panic("Could not open /usr/sbin/init\n");

    args.vas = pmap_create_vas(vm_get_ctx());
    args.argp = argv;
    args.envp = envp;

    tmp = loader_load(args.vas, init_bin, auxvp, 0, NULL, &init_range);
    if (tmp != 0)
        panic("Failed to load init\n");

    if (sched_new_td(auxvp->at_entry, true, args, &init_range, &init) != 0)
        panic("Failed to create init thread\n");

    sched_enqueue_td(init);
}

static void
sched_destroy_td(struct proc *td)
{
    struct vm_range *stack_range;
    struct vm_range *exec_range;
    vm_mapq_t *mapq;

    processor_free_pcb(td);
    stack_range = &td->addr_range[ADDR_RANGE_STACK];
    exec_range = &td->addr_range[ADDR_RANGE_EXEC];

    /*
     * User thread's have their stack allocated
     * with vm_alloc_pageframe() and kernel thread's
     * have their stacks allocated with dynalloc()
     */
    if (td->is_user) {
        vm_free_pageframe(stack_range->start, PROC_STACK_PAGES);
    } else {
        dynfree((void *)stack_range->start);
    }

    for (size_t i = 0; i < MTAB_ENTRIES; ++i) {
        mapq = &td->mapspace.mtab[i];
        vm_free_mapq(mapq);
    }

    for (size_t i = 0; i < PROC_MAX_FDS; ++i) {
        fd_close_fdnum(td, i);
    }

    loader_unload(td->addrsp, exec_range);
    pmap_free_vas(vm_get_ctx(), td->addrsp);
    dynfree(td);
}

/*
 * Cause an early preemption and lets
 * the next thread run.
 */
void
sched_rest(void)
{
    struct proc *td = this_td();

    if (td == NULL)
        return;

    td->rested = 1;
    sched_oneshot(true);
}

void
sched_exit(void)
{
    struct proc *td = this_td();
    struct vas kvas = vm_get_kvas();

    spinlock_acquire(&tdq_lock);
    intr_mask();

    /* Switch to kernel VAS and destroy td */
    pmap_switch_vas(vm_get_ctx(), kvas);
    sched_destroy_td(td);

    spinlock_release(&tdq_lock);
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
 *
 * Handles the transition from the currently running
 * thread to the next thread.
 */
void
sched_context_switch(struct trapframe *tf)
{
    struct cpu_info *ci = this_cpu();
    struct sched_state *state = &ci->sched_state;
    struct proc *next_td, *td = state->td;

    if (td != NULL) {
        signal_handle(state->td);
    }

    /*
     * If a thread is currently running and our policy is
     * MLFQ, then we should update the thread's priority.
     */
    if (td != NULL && policy == SCHED_POLICY_MLFQ) {
        td_pri_update(td);
    }

    /* Don't preempt if we have no threads */
    if ((next_td = sched_dequeue_td()) == NULL) {
        sched_oneshot(false);
        return;
    }

    /*
     * If we have a thread currently running, then we should save
     * its current register state and re-enqueue it.
     */
    if (td != NULL) {
        memcpy(td->tf, tf, sizeof(struct trapframe));
        sched_enqueue_td(td);
    }

    /* Perform the switch */
    memcpy(tf, next_td->tf, sizeof(struct trapframe));
    processor_switch_to(td, next_td);

    state->td = next_td;
    pmap_switch_vas(vm_get_ctx(), next_td->addrsp);
    sched_oneshot(false);
}

uint64_t
sys_exit(struct syscall_args *args)
{
    sched_exit();
    __builtin_unreachable();
}

void
sched_init(void)
{
    sched_init_queues();
    sched_load_init();
}

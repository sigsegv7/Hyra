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

/* In sys/<machine>/<machine>/switch.S */
void __sched_switch_to(struct trapframe *tf);

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
    size_t envp_len, argv_len, len;
    uintptr_t old_sp = 0;
    uintptr_t ret;

    for (envp_len = 0; envp[envp_len] != NULL; ++envp_len) {
        len = strlen(envp[envp_len]);
        sp = (void *)((uintptr_t)sp - len - 1);
        memcpy(sp, envp[envp_len], len);
    }

    for (argv_len = 0; argvp[argv_len] != NULL; ++argv_len) {
        len = strlen(argvp[argv_len]);
        sp = (void *)((uintptr_t)sp - len - 1);
        memcpy(sp, argvp[envp_len], len);
    }

    sp = (uint64_t *)(((uintptr_t)sp / 16) * 16);
    if (((argv_len + envp_len + 1) & 1) != 0) {
        sp--;
    }

    *--sp = 0;
    *--sp = 0;
    *--sp = auxv.at_entry;
    *--sp = AT_ENTRY;
    *--sp = auxv.at_phent;
    *--sp = AT_PHENT;
    *--sp = auxv.at_phnum;
    *--sp = AT_PHNUM;
    *--sp = auxv.at_phdr;
    *--sp = AT_PHDR;

    old_sp = (uintptr_t)sp;

    /* End of envp */
    *--sp = 0;
    sp -= envp_len;
    for (int i = 0; i < envp_len; ++i) {
        old_sp -= strlen(envp[i]) + 1;
        sp[i] = old_sp;
    }

    /* End of argvp */
    *--sp = 0;
    sp -= argv_len;
    for (int i = 0; i < argv_len; ++i) {
        old_sp -= strlen(argvp[i]) + 1;
        sp[i] = old_sp;
    }

    /* Argc */
    *--sp = argv_len;

    ret = (uintptr_t)stack_top;
    ret -= (ret - (uintptr_t)sp);
    return ret;
}

static uintptr_t
sched_create_stack(struct vas vas, bool user)
{
    int status;
    uintptr_t user_stack;
    const vm_prot_t USER_STACK_PROT = PROT_WRITE | PROT_USER;

    if (!user) {
        return (uintptr_t)dynalloc(STACK_SIZE);
    }

    user_stack = vm_alloc_pageframe(STACK_PAGES);

    status = vm_map_create(vas, user_stack, user_stack, USER_STACK_PROT,
                           STACK_SIZE);

    if (status != 0) {
        return 0;
    }

    memset(PHYS_TO_VIRT(user_stack), 0, STACK_SIZE);
    return user_stack;
}

static struct proc *
sched_create_td(uintptr_t rip, char *argvp[], char *envp[], struct auxval auxv,
                struct vas vas, bool is_user)
{
    struct proc *td;
    uintptr_t stack, sp;
    void *stack_virt;
    struct trapframe *tf;

    tf = dynalloc(sizeof(struct trapframe));
    if (tf == NULL) {
        return NULL;
    }

    stack = sched_create_stack(vas, is_user);
    if (stack == 0) {
        dynfree(tf);
        return NULL;
    }

    stack_virt = (is_user) ? PHYS_TO_VIRT(stack) : (void *)stack;

    td = dynalloc(sizeof(struct proc));
    if (td == NULL) {
        /* TODO: Free stack */
        dynfree(tf);
        return NULL;
    }

    memset(tf, 0, sizeof(struct trapframe));
    sp = sched_init_stack((void *)((uintptr_t)stack_virt + STACK_SIZE),
                          argvp, envp, auxv);

    /* Setup process itself */
    td->pid = 0;        /* Don't assign PID until enqueued */
    td->cpu = NULL;     /* Not yet assigned a core */
    td->tf = tf;
    td->addrsp = vas;

    /* Setup trapframe */
    if (!is_user) {
        init_frame(tf, rip, sp);
    } else {
        init_frame_user(tf, rip, VIRT_TO_PHYS(sp));
    }
    return td;
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

    if (state->td != NULL) {
        /* Save our trapframe */
        td = state->td;
        memcpy(td->tf, tf, sizeof(struct trapframe));
    }

    /* Copy to stack */
    memcpy(tf, next_td->tf, sizeof(struct trapframe));

    td = state->td;
    state->td = next_td;

    if (td != NULL) {
        sched_enqueue_td(td);
    }

    pmap_switch_vas(vm_get_ctx(), next_td->addrsp);
    sched_oneshot();
}

void
sched_init(void)
{
    struct proc *init;
    struct auxval auxv = {0};
    struct vas vas = pmap_create_vas(vm_get_ctx());
    const char *init_bin;

    int status;
    char *ld_path;
    char *argv[] = {"/boot/init", NULL};
    char *envp[] = {NULL};

    TAILQ_INIT(&td_queue);

    if ((init_bin = initramfs_open("/boot/init")) == NULL) {
        panic("Could not open /boot/init\n");
    }

    status = loader_load(vas, init_bin, &auxv, 0, &ld_path);
    if (status != 0) {
        panic("Could not load init\n");
    }

    init = sched_create_td((uintptr_t)auxv.at_entry, argv, envp, auxv, vas, true);
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

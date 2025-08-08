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

#include <sys/spinlock.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <dev/timer.h>
#include <uacpi/kernel_api.h>
#include <uacpi/platform/arch_helpers.h>
#include <uacpi/types.h>
#include <uacpi/event.h>
#include <uacpi/sleep.h>
#include <machine/cdefs.h>
#include <machine/pio.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#if defined(__x86_64__)
#include <machine/idt.h>
#include <machine/ioapic.h>
#endif  /* __x86_64__ */
#include <dev/acpi/uacpi.h>
#include <dev/acpi/acpi.h>
#include <dev/pci/pci.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <string.h>

typedef struct {
    uacpi_io_addr base;
    uacpi_size length;
} io_range_t;

struct uacpi_work {
    uacpi_work_handler hand;
    uacpi_handle ctx;
    TAILQ_ENTRY(uacpi_work) link;
};

uacpi_status
uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler h, uacpi_handle ctx);

extern struct proc g_proc0;

static struct proc *event_td;
static TAILQ_HEAD(, uacpi_work) acpi_gpe_eventq;
static TAILQ_HEAD(, uacpi_work) acpi_notify_eventq;

/*
 * Dispatch ACPI general purpose events from
 * hardware.
 */
static void
uacpi_gpe_dispatch(void)
{
    struct uacpi_work *work;

    work = TAILQ_FIRST(&acpi_gpe_eventq);
    if (work == NULL) {
        return;
    }

    work->hand(work->ctx);
    TAILQ_REMOVE(&acpi_gpe_eventq, work, link);
    dynfree(work);
}

/*
 * Dispatch ACPI general notify events.
 */
static void
uacpi_notify_dispatch(void)
{
    struct uacpi_work *work;

    work = TAILQ_FIRST(&acpi_notify_eventq);
    if (work == NULL) {
        return;
    }

    work->hand(work->ctx);
    TAILQ_REMOVE(&acpi_gpe_eventq, work, link);
    dynfree(work);
}

static void
uacpi_event_td(void)
{
    for (;;) {
        uacpi_gpe_dispatch();
        uacpi_notify_dispatch();
        sched_yield();
    }
}

static void
shutdown(uacpi_handle ctx)
{
    kprintf("power button pressed\n");
    kprintf("halting machine...\n");
    cpu_halt_all();
}

static uacpi_interrupt_ret
power_button_handler(uacpi_handle ctx)
{
    md_intoff();
    uacpi_kernel_schedule_work(UACPI_WORK_GPE_EXECUTION, shutdown, NULL);
    md_inton();

    for (;;) {
        md_hlt();
    }

    __builtin_unreachable();
}

void *
uacpi_kernel_alloc(uacpi_size size)
{
    return dynalloc(size);
}

void
uacpi_kernel_free(void *mem)
{
    dynfree(mem);
}

uacpi_status
uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address)
{
    paddr_t pa;

    pa = acpi_rsdp();
    if (pa == 0) {
        return UACPI_STATUS_NOT_FOUND;
    }

    *out_rsdp_address = pa;
    return UACPI_STATUS_OK;
}

/* TODO: Actual mutex */
uacpi_handle
uacpi_kernel_create_mutex(void)
{
    struct spinlock *lp;

    lp = dynalloc(sizeof(*lp));
    if (lp == NULL) {
        return NULL;
    }
    memset(lp, 0, sizeof(*lp));
    return lp;
}

void
uacpi_kernel_free_mutex(uacpi_handle handle)
{
    dynfree(handle);
}

uacpi_status
uacpi_kernel_acquire_mutex(uacpi_handle handle, [[maybe_unused]] uacpi_u16 timeout)
{
    spinlock_acquire((struct spinlock *)handle);
    return UACPI_STATUS_OK;
}

void
uacpi_kernel_release_mutex(uacpi_handle handle)
{
    spinlock_release((struct spinlock *)handle);
}

uacpi_thread_id
uacpi_kernel_get_thread_id(void)
{
    struct proc *td = this_td();

    if (td == NULL) {
        return 0;   /* PID 0 */
    }

    return &td->pid;
}

uacpi_status
uacpi_kernel_handle_firmware_request(uacpi_firmware_request *request)
{
    switch (request->type) {
    case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
        panic("uacpi: fatal firmware request\n");
        break;
    }

    return UACPI_STATUS_OK;
}

uacpi_handle
uacpi_kernel_create_spinlock(void)
{
    struct spinlock *lp;

    lp = dynalloc(sizeof(*lp));
    if (lp == NULL) {
        return NULL;
    }
    memset(lp, 0, sizeof(*lp));
    return lp;
}

void
uacpi_kernel_free_spinlock(uacpi_handle lock)
{
    dynfree(lock);
}

uacpi_cpu_flags
uacpi_kernel_lock_spinlock(uacpi_handle lock)
{
    struct spinlock *lp = lock;

    return __atomic_test_and_set(&lp->lock, __ATOMIC_ACQUIRE);
}

void
uacpi_kernel_unlock_spinlock(uacpi_handle lock, uacpi_cpu_flags interrupt_state)
{
    spinlock_release((struct spinlock *)lock);
}

uacpi_handle
uacpi_kernel_create_event(void)
{
    size_t *counter;

    counter = dynalloc(sizeof(*counter));
    if (counter == NULL) {
        return NULL;
    }

    *counter = 0;
    return counter;
}

void
uacpi_kernel_free_event(uacpi_handle handle)
{
    dynfree(handle);
}

uacpi_bool
uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout)
{
    size_t *counter = (size_t *)handle;
    struct timer tmr;
    size_t usec_start, usec;
    size_t elapsed_msec;

    if (timeout == 0xFFFF) {
        while (*counter != 0) {
            md_pause();
        }
        return UACPI_TRUE;
    }

    req_timer(TIMER_GP, &tmr);
    usec_start = tmr.get_time_usec();

    for (;;) {
        if (*counter == 0) {
            return UACPI_TRUE;
        }

        usec = tmr.get_time_usec();
        elapsed_msec = (usec - usec_start) / 1000;
        if (elapsed_msec >= timeout) {
            break;
        }

        md_pause();
    }

    __atomic_fetch_sub((size_t *)handle, 1, __ATOMIC_SEQ_CST);
    return UACPI_FALSE;
}

void
uacpi_kernel_signal_event(uacpi_handle handle)
{
    __atomic_fetch_add((size_t *)handle, 1, __ATOMIC_SEQ_CST);
}

void
uacpi_kernel_reset_event(uacpi_handle handle)
{
    __atomic_store_n((size_t *)handle, 0, __ATOMIC_SEQ_CST);
}

uacpi_status
uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler fn,
    uacpi_handle ctx, uacpi_handle *out_irq_handle)
{
    struct intr_hand ih;

    ih.func = (void *)fn;
    ih.priority = IPL_HIGH;
    ih.irq = irq;
    if (intr_register("acpi", &ih) == NULL) {
        return UACPI_STATUS_INTERNAL_ERROR;
    }

    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_uninstall_interrupt_handler([[maybe_unused]] uacpi_interrupt_handler fn, uacpi_handle irq_handle)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler h, uacpi_handle ctx)
{
    struct uacpi_work *work;

    work = dynalloc(sizeof(*work));
    if (work == NULL) {
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    work->hand = h;
    work->ctx = ctx;

    switch (type) {
    case UACPI_WORK_GPE_EXECUTION:
        TAILQ_INSERT_TAIL(&acpi_gpe_eventq, work, link);
        break;
    case UACPI_WORK_NOTIFICATION:
        TAILQ_INSERT_TAIL(&acpi_notify_eventq, work, link);
        break;
    }

    return 0;
}

uacpi_status
uacpi_kernel_wait_for_work_completion(void)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}

void uacpi_kernel_stall(uacpi_u8 usec)
{
    /* XXX: STUB */
    (void)usec;
}

void
uacpi_kernel_sleep(uacpi_u64 msec)
{
    struct timer tmr;

    req_timer(TIMER_GP, &tmr);
    tmr.msleep(msec);
}

void *
uacpi_kernel_map(uacpi_phys_addr addr, [[maybe_unused]] uacpi_size len)
{
    return PHYS_TO_VIRT(addr);
}

void
uacpi_kernel_unmap([[maybe_unused]] void *addr, [[maybe_unused]] uacpi_size len)
{
    /* XXX: no-op */
    (void)addr;
    (void)len;
}

uacpi_status
uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value)
{
    io_range_t *rp = (io_range_t *)handle;

    if (offset >= rp->length) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    *out_value = inb(rp->base + offset);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value)
{
    io_range_t *rp = (io_range_t *)handle;

    if (offset >= rp->length) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    *out_value = inw(rp->base + offset);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value)
{
    io_range_t *rp = (io_range_t *)handle;

    if (offset >= rp->length) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    *out_value = inl(rp->base + offset);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value)
{
    io_range_t *rp = (io_range_t *)handle;

    if (offset >= rp->length) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    outb(rp->base + offset, in_value);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value)
{
    io_range_t *rp = (io_range_t *)handle;

    if (offset >= rp->length) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    outw(rp->base + offset, in_value);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value)
{
    io_range_t *rp = (io_range_t *)handle;

    if (offset >= rp->length) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    outl(rp->base + offset, in_value);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle)
{
    io_range_t *rp;

    rp = dynalloc(sizeof(*rp));
    if (rp == NULL) {
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    rp->base = base;
    rp->length = len;
    *out_handle = rp;
    return UACPI_STATUS_OK;
}

void
uacpi_kernel_io_unmap(uacpi_handle handle)
{
    dynfree(handle);
}

void
uacpi_kernel_pci_device_close([[maybe_unused]] uacpi_handle handle)
{
    /* XXX: no-op */
    (void)handle;
}

uacpi_status
uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle *out_handle)
{
    struct pci_device *devp;

    devp = dynalloc(sizeof(*devp));
    if (devp == NULL) {
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    devp->segment = address.segment;
    devp->bus = address.bus;
    devp->slot = address.device;
    devp->func = address.function;
    pci_add_device(devp);

    *out_handle = devp;
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value)
{
    struct pci_device *devp = handle;
    uint32_t v;

    v = pci_readl(devp, offset);
    *out_value = (v >> ((offset & 3) * 8)) & MASK(8);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value)
{
    struct pci_device *devp = handle;
    uint32_t v;

    v = pci_readl(devp, offset);
    *out_value = (v >> ((offset & 2) * 8)) & MASK(16);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value)
{
    struct pci_device *devp = handle;
    *out_value = pci_readl(devp, offset);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value)
{
    struct pci_device *devp = handle;
    uint32_t v;

    uacpi_kernel_pci_read8(handle, offset, (void *)&v);
    v &= ~(0xFFFF >> ((offset & 3) * 8));
    v |= (in_value >> ((offset & 3) * 8));
    pci_writel(devp, offset, v);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value)
{
    struct pci_device *devp = handle;
    uint32_t v;

    uacpi_kernel_pci_read8(handle, offset, (void *)&v);
    v &= ~(0xFFFF >> ((offset & 2) * 8));
    v |= (in_value >> ((offset & 2) * 8));
    pci_writel(devp, offset, v);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value)
{
    struct pci_device *devp = handle;

    pci_writel(devp, offset, in_value);
    return UACPI_STATUS_OK;
}

uacpi_u64
uacpi_kernel_get_nanoseconds_since_boot(void)
{
    static uacpi_u64 time = 0;
    static struct timer tmr = {0};
    tmrr_status_t tmr_error;

    if (time == 0) {
        tmr_error = req_timer(TIMER_GP, &tmr);
        if (tmr_error != TMRR_SUCCESS) {
            time += 1000000;
            return time;
        }
    }

    time = tmr.get_time_nsec();
    return time;
}

void
uacpi_kernel_log(uacpi_log_level level, const uacpi_char *p)
{
    kprintf(p);
}

int
uacpi_init(void)
{
    uacpi_status ret;

    ret = uacpi_initialize(0);
    if (uacpi_unlikely_error(ret)) {
        kprintf("uacpi init error: %s\n", uacpi_status_to_string(ret));
        return -1;
    }

    ret = uacpi_namespace_load();
    if (uacpi_unlikely_error(ret)) {
        kprintf("uacpi namespace load error: %s\n", uacpi_status_to_string(ret));
        return -1;
    }

    ret = uacpi_namespace_initialize();
    if (uacpi_unlikely_error(ret)) {
        kprintf("uacpi namespace init error: %s\n", uacpi_status_to_string(ret));
        return -1;
    }

    ret = uacpi_finalize_gpe_initialization();
    if (uacpi_unlikely_error(ret)) {
        kprintf("uacpi GPE init error: %s\n", uacpi_status_to_string(ret));
        return -1;
    }

    ret = uacpi_install_fixed_event_handler(
        UACPI_FIXED_EVENT_POWER_BUTTON,
        power_button_handler, UACPI_NULL
    );

    if (uacpi_unlikely_error(ret)) {
        kprintf("failed to install power button event: %s\n",
            uacpi_status_to_string(ret)
        );
        return -1;
    }

    TAILQ_INIT(&acpi_gpe_eventq);
    TAILQ_INIT(&acpi_notify_eventq);
    spawn(&g_proc0, uacpi_event_td, NULL, 0, &event_td);
    return 0;
}

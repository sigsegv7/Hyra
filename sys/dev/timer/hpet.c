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

#include <dev/timer/hpet.h>
#include <firmware/acpi/acpi.h>
#include <sys/panic.h>
#include <sys/cdefs.h>
#include <sys/timer.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/mmio.h>

__MODULE_NAME("hpet");
__KERNEL_META("$Hyra$: hpet.c, Ian Marco Moffett, "
              "HPET driver");

#define HPET_REG_CAPS               0x00
#define HPET_GENERAL_CONFIG         0x10
#define HPET_REG_MAIN_COUNTER       0xF0

#define CAP_REV_ID(caps)            __SHIFTOUT(caps, 0xFF)
#define CAP_NUM_TIM(caps)           __SHIFTOUT(caps, 0x1F << 8)
#define CAP_COUNT_SIZE(caps)        __SHIFTOUT(caps, __BIT(13))
#define CAP_VENDOR_ID(caps)         __SHIFTOUT(caps, 0xFFFF << 16)
#define CAP_CLK_PERIOD(caps)        (caps >> 32)

/*
 * TODO: Use some sort of helper function that
 *       uses PAUSE or some similar instruction
 *       for the current architecture. It is
 *       *not* a good idea to spin for a prolonged
 *       amount of time without it. For now we'll
 *       just use PAUSE on x86_64 like this.
 */
#if defined(__x86_64__)
#define spinwait_hint()   __ASMV("pause")
#else
#define spinwait_hint()   __nothing
#endif  /* defined(__x86_64__) */

static struct timer timer = { 0 };
static struct hpet *acpi_hpet = NULL;
static void *hpet_base = NULL;
static bool is_faulty = false;

/*
 * Read from HPET register space.
 *
 * @reg: Register to read from.
 */
static inline uint64_t
hpet_read(uint32_t reg)
{
    void *addr;

    addr = (void *)((uintptr_t)hpet_base + reg);
    return mmio_read64(addr);
}

/*
 * Write to HPET register space.
 *
 * @reg: Register to write to.
 * @val: Value to write.
 */
static inline void
hpet_write(uint32_t reg, uint64_t val)
{
    void *addr;

    addr = (void *)((uintptr_t)hpet_base + reg);
    mmio_write64(addr, val);
}

static int
hpet_sleep(uint64_t n, uint64_t units)
{
    uint64_t caps;
    uint32_t period;
    uint64_t counter_val;
    volatile size_t ticks;

    /* Don't even try if faulty, would probably cause deadlock */
    if (is_faulty) {
        return EXIT_FAILURE;
    }

    caps = hpet_read(HPET_REG_CAPS);
    period = CAP_CLK_PERIOD(caps);
    counter_val = hpet_read(HPET_REG_MAIN_COUNTER);

    ticks = counter_val + (n * (units / period));

    while (hpet_read(HPET_REG_MAIN_COUNTER) < ticks) {
        spinwait_hint();
    }

    return EXIT_SUCCESS;
}

int
hpet_msleep(size_t ms)
{
    return hpet_sleep(ms, 1000000000000);
}

int
hpet_usleep(size_t us)
{
    return hpet_sleep(us, 1000000000);
}

int
hpet_nsleep(size_t ns)
{
    return hpet_sleep(ns, 1000000);
}

int
hpet_init(void)
{
    struct acpi_gas *gas;
    uint64_t caps;

    acpi_hpet = acpi_query("HPET");
    if (acpi_hpet == NULL)
        return 1;       /* Not found */

    gas = &acpi_hpet->gas;
    hpet_base = (void *)gas->address;
    caps = hpet_read(HPET_REG_CAPS);

    /* Ensure caps aren't bogus */
    if (CAP_REV_ID(caps) == 0) {
        KERR("Found bogus revision, assuming faulty\n");
        is_faulty = true;
        return 1;
    }
    if (CAP_CLK_PERIOD(caps) > 0x05F5E100) {
        /*
         * The spec states this counter clk period
         * must be <= 0x05F5E100. So we'll consider it
         * as bogus if it exceeds this value
         */
        KERR("Found bogus COUNTER_CLK_PERIOD, assuming faulty\n");
        KINFO("HPET REV - 0x%x\n", CAP_REV_ID(caps));
        KINFO("COUNTER_CLK_PERIOD - 0x%x\n", CAP_CLK_PERIOD(caps));
        is_faulty = true;
        return 1;
    }

    KINFO("HPET integrity verified\n");

    hpet_write(HPET_REG_MAIN_COUNTER, 0);
    hpet_write(HPET_GENERAL_CONFIG, 1);

    /* Setup the timer descriptor */
    timer.name = "HIGH_PRECISION_EVENT_TIMER";
    timer.msleep = hpet_msleep;
    timer.usleep = hpet_usleep;
    timer.nsleep = hpet_nsleep;
    register_timer(TIMER_GP, &timer);

    /* Not faulty */
    is_faulty = false;
    return 0;
}

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

#include <sys/types.h>
#include <sys/mmio.h>
#include <sys/syslog.h>
#include <machine/hpet.h>
#include <dev/acpi/acpi.h>
#include <dev/acpi/tables.h>
#include <dev/timer.h>

#define pr_trace(fmt, ...) kprintf("hpet: " fmt, ##__VA_ARGS__)
#define pr_error(fmt, ...) pr_trace(__VA_ARGS__)

#define HPET_REG_CAPS               0x00
#define HPET_GENERAL_CONFIG         0x10
#define HPET_REG_MAIN_COUNTER       0xF0

#define CAP_REV_ID(caps)            (caps & 0xFF)
#define CAP_NUM_TIM(caps)           (caps >> 8) & 0x1F
#define CAP_CLK_PERIOD(caps)        (caps >> 32)

#define FSEC_PER_SECOND 1000000000000000ULL
#define NSEC_PER_SECOND 1000000000ULL
#define USEC_PER_SECOND 1000000ULL

static void *hpet_base = NULL;
static struct timer timer = {0};

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

    caps = hpet_read(HPET_REG_CAPS);
    period = CAP_CLK_PERIOD(caps);
    counter_val = hpet_read(HPET_REG_MAIN_COUNTER);

    ticks = counter_val + (n * (units / period));

    while (hpet_read(HPET_REG_MAIN_COUNTER) < ticks) {
        __ASMV("rep; nop");
    }

    return 0;
}

static int
hpet_msleep(size_t ms)
{
    return hpet_sleep(ms, 1000000000000);
}

static int
hpet_usleep(size_t us)
{
    return hpet_sleep(us, 1000000000);
}

static int
hpet_nsleep(size_t ns)
{
    return hpet_sleep(ns, 1000000);
}

static size_t
hpet_time_usec(void)
{
    uint64_t period, freq, caps;
    uint64_t counter;

    caps = hpet_read(HPET_REG_CAPS);
    period = CAP_CLK_PERIOD(caps);
    freq = FSEC_PER_SECOND / period;

    counter = hpet_read(HPET_REG_MAIN_COUNTER);
    return (counter * USEC_PER_SECOND) / freq;
}

static size_t
hpet_time_nsec(void)
{
    uint64_t period, freq, caps;
    uint64_t counter;

    caps = hpet_read(HPET_REG_CAPS);
    period = CAP_CLK_PERIOD(caps);
    freq = FSEC_PER_SECOND / period;

    counter = hpet_read(HPET_REG_MAIN_COUNTER);
    return (counter * NSEC_PER_SECOND) / freq;
}

static size_t
hpet_time_sec(void)
{
    return hpet_time_usec() / USEC_PER_SECOND;
}

int
hpet_init(void)
{
    struct acpi_gas *gas;
    struct acpi_hpet *hpet;
    uint64_t caps;

    hpet = acpi_query("HPET");
    if (hpet == NULL)
        return -1;

    gas = &hpet->gas;
    hpet_base = (void *)gas->address;

    /* Ensure caps aren't bogus */
    caps = hpet_read(HPET_REG_CAPS);
    if (CAP_REV_ID(caps) == 0) {
        pr_error("found bogus revision, assuming faulty\n");
        return -1;
    }
    if (CAP_CLK_PERIOD(caps) > 0x05F5E100) {
        /*
         * The spec states this counter clk period must
         * be <= 0x05F5E100. So we'll consider it as bogus
         * if it exceeds this value
         */
        pr_error("found bogus COUNTER_CLK_PERIOD, assuming faulty\n");
        return 1;
    }

    pr_trace("HPET integrity verified\n");
    hpet_write(HPET_REG_MAIN_COUNTER, 0);
    hpet_write(HPET_GENERAL_CONFIG, 1);

    /* Setup timer descriptor */
    timer.name = "HIGH_PRECISION_EVENT_TIMER";
    timer.msleep = hpet_msleep;
    timer.usleep = hpet_usleep;
    timer.nsleep = hpet_nsleep;
    timer.get_time_usec = hpet_time_usec;
    timer.get_time_nsec = hpet_time_nsec;
    timer.get_time_sec = hpet_time_sec;
    timer.flags = TIMER_MONOTONIC;
    register_timer(TIMER_GP, &timer);
    return 0;
}

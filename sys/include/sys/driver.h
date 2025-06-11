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

#ifndef _SYS_DRIVER_H_
#define _SYS_DRIVER_H_

#include <sys/cdefs.h>
#include <sys/proc.h>
#include <sys/types.h>

#if defined(_KERNEL)

/* Variable driver data */
struct driver_var {
    uint8_t deferred : 1;
};

struct driver {
    int(*init)(void);
    struct driver_var *data;
};

extern struct proc g_proc0;

/* Early (high priority) drivers */
extern char __drivers_init_start[];
extern char __drivers_init_end[];

/* Deferred (low priority) drivers */
extern char __driversd_init_start[];
extern char __driversd_init_end[];

#define DRIVER_EXPORT(INIT)                         \
    static struct driver_var __driver_var = {       \
        .deferred = 0                               \
    };                                              \
                                                    \
    __attribute__((used, section(".drivers")))      \
    static struct driver __driver_desc = {          \
        .init = INIT,                               \
        .data = &__driver_var                       \
    }

/*
 * Some drivers are not required to start up
 * early for proper system operation and may
 * be deferred to start at a later time.
 *
 * Examples of such (deferrable) drivers include code
 * that waits for I/O (e.g., disks, network cards,
 * et cetera). This allows for faster boot times
 * as only *required* drivers are started before
 * everything else.
 *
 * Drivers that wish to be deferred may export themselves
 * via the DRIVER_DEFER() macro. The DRIVER_DEFERRED()
 * macro gives the value of 1 if the current driver
 * context has yet to be initialized. The driver may
 * use this to defer requests for I/O.
 */
#define DRIVER_DEFER(INIT)                           \
    static struct driver_var __driver_var = {        \
        .deferred = 1                                \
    };                                               \
                                                     \
    __attribute__((used, section(".drivers.defer"))) \
    static struct driver __driver_desc = {           \
        .init = INIT,                                \
        .data = &__driver_var                        \
    }

#define DRIVER_DEFERRED() __driver_var.deferred

#define DRIVERS_INIT() \
    for (struct driver *__d = (struct driver *)__drivers_init_start;    \
         (uintptr_t)__d < (uintptr_t)__drivers_init_end; ++__d)         \
    {                                                                   \
        __d->init();                                                    \
    }

#define DRIVERS_SCHED() \
    spawn(&g_proc0, __driver_init_td, NULL, 0, NULL)

void __driver_init_td(void);

#endif  /* _KERNEL */
#endif  /* !_SYS_DRIVER_H_ */

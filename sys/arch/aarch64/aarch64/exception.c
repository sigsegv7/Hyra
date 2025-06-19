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

#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <machine/cdefs.h>
#include <machine/exception.h>

#define pr_trace(fmt, ...) kprintf("exception: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

static inline void
log_esr_class(uint8_t class)
{
    switch (class) {
    case EC_WF:
        pr_error("trapped WF\n");
        break;
    case EC_MCRMRC:
        pr_error("trapped MCR/MRC\n");
        break;
    case EC_MCRRC:
        pr_trace("trapped MCRR/MRRC\n");
        break;
    case EC_LDCSTC:
        pr_error("trapped LDC/STC\n");
        break;
    case EC_SVE:
        pr_trace("trapped SVE/SIMD/FP operation\n");
        break;
    case EC_BRE:
        pr_error("ibt: bad branch target\n");
        break;
    case EC_ILLX:
        pr_error("illegal execution state\n");
        break;
    case EC_SVC64:
        /* TODO */
        pr_error("supervisor call (TODO)!!\n");
        break;
    case EC_PCALIGN:
        pr_error("PC alignment fault\n");
        break;
    case EC_DABORT:
    case EC_EDABORT:
        pr_error("data abort\n");
        break;
    case EC_SPALIGN:
        pr_error("SP alignment fault\n");
        break;
    case EC_SERR:
        pr_error("system error\n");
        break;
    default:
        pr_error("unknown exception\n");
    }
}

static void
regdump(struct trapframe *tf, uint64_t elr)
{
    kprintf(OMIT_TIMESTAMP
        "X0=%p X1=%p X2=%p\n"
        "X3=%p X4=%p X5=%p\n"
        "X6=%p X7=%p X8=%p\n"
        "X9=%p X10=%p X11=%p\n"
        "X12=%p X13=%p X14=%p\n"
        "X15=%p X16=%p X17=%p\n"
        "X18=%p X19=%p X20=%p\n"
        "X21=%p X22=%p X23=%p\n"
        "X24=%p X25=%p X26=%p\n"
        "X27=%p X28=%p X29=%p\n"
        "X30=%p\n"
        "ELR=%p\n",
        tf->x0, tf->x1, tf->x2, tf->x3,
        tf->x4, tf->x5, tf->x6, tf->x7,
        tf->x8, tf->x9, tf->x10, tf->x11,
        tf->x12, tf->x13, tf->x14, tf->x15,
        tf->x16, tf->x17, tf->x18, tf->x19,
        tf->x20, tf->x21, tf->x22, tf->x23,
        tf->x24, tf->x25, tf->x26, tf->x27,
        tf->x28, tf->x29, tf->x30, elr);
}

/*
 * Handle an exception
 *
 * @esr: Copy of the Exception Syndrome Register
 */
void
handle_exception(struct trapframe *tf)
{
    uint8_t class;

    class = (tf->esr >> 26) & 0x3F;
    log_esr_class(class);
    regdump(tf, tf->elr);
    for (;;) {
        md_hlt();
    }
}

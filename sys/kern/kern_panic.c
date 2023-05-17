/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

#include <sys/panic.h>
#include <sys/types.h>
#include <sys/printk.h>
#include <sys/aal.h>
#include <vt/vt.h>
#if defined(__aarch64__)
# include <arch/aarch64/board.h>
#endif

#define MAX_FRAMES 15

#define SCREEN_BG 0x000080
#define SCREEN_FG 0xF2F5E5
#define ASCII_SEP "----------------------------------" \
                  "----------------------------------"

extern struct vt_descriptor g_vt;

struct kernel_symbol {
	uint64_t addr;
	char* name;
};

extern struct kernel_symbol g_ksym_table[];

/*
 * Does a stacktrace.
 *
 * TODO: Add support with other arches.
 */


static const char *
panic_resolve_name(uintptr_t addr, size_t *offset)
{
    uintptr_t prev_addr = 0;
    const char *prev_name = NULL;

    for (size_t i = 0;; ++i) {
        if (g_ksym_table[i].addr > addr) {
            *offset = addr - prev_addr;
            return prev_name;
        }

        prev_addr = g_ksym_table[i].addr;
        prev_name = g_ksym_table[i].name;

        if (g_ksym_table[i].addr == (uintptr_t)-1) {
            return NULL;
        }
    }

    __builtin_unreachable();
}

static void
panic_stacktrace(void)
{
    uintptr_t *bp = NULL;       /* Also counts as `fp` in AARCH64 */

#if defined(__x86_64__)
    __asm("mov %%rbp, %0"
          : "=r" (bp)
          :
          : "memory"
    );
#elif defined(__aarch64__)
    __asm("mov %0, x29\n"
          : "=r" (bp)
          :
          : "memory"
    );
#endif

    uint8_t frame_count = 0; 
    size_t offset;

    while (bp != NULL && frame_count < MAX_FRAMES) {
        const char *name = panic_resolve_name(bp[1], &offset);

        if (name == NULL) {
            printk("* [%d] 0x%x\n", frame_count, bp[1]);
        } else {
            printk("* [%d] %s <0x%x+0x%x>\n",
                   frame_count, name, bp[1], offset);
        }

        bp = (uintptr_t *)bp[0];
        ++frame_count;
    }
}

/*
 * Writes out diagnostic information
 * to the console.
 */

static void
panic_log_diag(const char *fmt)
{
    printk("\n\nSomething went wrong with Vega "
           "and your PC requires a restart.\n"); 

    printk("Below is some technical information, "
            "if you aren't a developer you can\n"
            "either send us a picture or ignore this.\n\n");

    printk("** Technical information **\n\n");
    
    printk("Traversing call stack...\n");
    printk("%s\n", ASCII_SEP);

    panic_stacktrace();
    printk("%s\n\n", ASCII_SEP);
#if defined(__aarch64__)
    printk("Board: %s\n", aarch64_get_board());
#endif
    
    printk("Vega revision: v%s\n\n", VEGA_VERSION);

    printk("kpanic - halting: "); 
}

__dead void
panic(const char *fmt, ...)
{
    struct vt_attr panic_attr = {
        .bg = SCREEN_BG,
        .text_bg = SCREEN_BG,
        .text_fg = SCREEN_FG,
        .cursor_type = CURSOR_NONE
    };
    
    vt_chattr(&g_vt, &panic_attr);
    vt_reset(&g_vt);

    va_list ap;
    va_start(ap, fmt);
    panic_log_diag(fmt);
    vprintk(fmt, &ap);

    /* Disable the processor */
    __irq_disable();
    halt();
    __builtin_unreachable();
}

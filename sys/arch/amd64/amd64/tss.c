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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <vm/dynalloc.h>
#include <vm/physmem.h>
#include <vm/vm.h>
#include <machine/tss.h>
#include <machine/cpu.h>
#include <assert.h>
#include <string.h>

/*
 * Allocates memory for TSS and kernel
 * stack.
 *
 * XXX: Kernel stack is allocated from
 *      vm_alloc_frame()
 */
static void
alloc_resources(struct cpu_info *ci)
{
    const size_t STACK_SIZE = 0x1000;
    struct tss_entry *tss;
    static uintptr_t rsp0_base, rsp0;

    if (ci->tss == NULL) {
        tss = dynalloc(sizeof(*tss));

        if (tss == NULL) {
            panic("failed to alloc TSS\n");
        }

        memset(tss, 0, sizeof(*tss));
        rsp0_base = vm_alloc_frame(1) + VM_HIGHER_HALF;

        if (rsp0_base == 0) {
            panic("could not allocate RSP0 base\n");
        }

        rsp0 = rsp0_base + STACK_SIZE;
        tss->rsp0_lo = rsp0 & 0xFFFFFFFF;
        tss->rsp0_hi = (rsp0 >> 32) & 0xFFFFFFFF;
        ci->tss = tss;
    }
}

/*
 * Update interrupt stack table entry `istno' with `stack'
 *
 * @stack: Interrupt stack.
 * @istno: IST number, must be 1-based.
 *
 * Returns 0 on success.
 */
int
tss_update_ist(struct cpu_info *ci, union tss_stack stack, uint8_t istno)
{
    volatile struct tss_entry *tss = ci->tss;

    __assert(tss != NULL);

    switch (istno) {
    case 1:
        tss->ist1_lo = stack.top_lo;
        tss->ist1_hi = stack.top_hi;
        break;
    case 2:
        tss->ist2_lo = stack.top_lo;
        tss->ist2_hi = stack.top_hi;
        break;
    case 3:
        tss->ist3_lo = stack.top_lo;
        tss->ist3_hi = stack.top_hi;
        break;
    case 4:
        tss->ist4_lo = stack.top_lo;
        tss->ist4_hi = stack.top_hi;
        break;
    case 5:
        tss->ist5_lo = stack.top_lo;
        tss->ist5_hi = stack.top_hi;
        break;
    case 6:
        tss->ist6_lo = stack.top_lo;
        tss->ist6_hi = stack.top_hi;
        break;
    case 7:
        tss->ist7_lo = stack.top_lo;
        tss->ist7_hi = stack.top_hi;
        break;
    default:
        return -EINVAL;
    };

    return 0;
}

/*
 * Allocates TSS stack.
 *
 * @entry_out: Pointer to location where allocated entry
 *             will be sent.
 *
 * Returns 0 on success.
 */
int
tss_alloc_stack(union tss_stack *entry_out, size_t size)
{
    uintptr_t base = (uintptr_t)dynalloc(size);

    if (base == 0) {
        return -ENOMEM;
    }

    entry_out->top = base + size;
    return 0;
}

void
write_tss(struct cpu_info *ci, struct tss_desc *desc)
{
    volatile struct tss_entry *tss;
    uintptr_t tss_base;

    alloc_resources(ci);
    tss_base = (uintptr_t)ci->tss;

    /*
     * XXX: The AVL (Available for use by system software)
     *      bit is ignored by hardware and it is up to us
     *      to decide how to use it... As of now, it is useless
     *      to us and shall remain 0.
     */
    desc->seglimit = sizeof(struct tss_entry);
    desc->p = 1;        /* Must be present to be valid! */
    desc->g = 0;        /* Granularity -> 0 */
    desc->avl = 0;      /* Not used */
    desc->dpl = 0;      /* Descriptor Privilege Level -> 0 */
    desc->type = 0x9;   /* For TSS -> 0x9 (0b1001) */

    desc->base_lo16 =       tss_base & 0xFFFF;
    desc->base_mid8 =       (tss_base >> 16) & 0xFF;
    desc->base_hi_mid8 =    (tss_base >> 24) & 0xFF;
    desc->base_hi32 =       (tss_base >> 32) & 0xFFFFFFFF;

    tss = ci->tss;
    tss->io_base = 0xFF;    /* Disallow ring 3 port I/O  */
}

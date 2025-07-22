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

#ifndef _OEMU_CPU_H_
#define _OEMU_CPU_H_

#include <sys/types.h>
#include <sys/param.h>
#include <stdint.h>
#include <stddef.h>
#include <oemu/types.h>

#define MEMORY_SIZE 512

/*
 * Processor state register
 */
#define CPU_SRS_SV      BIT(1)  /* Supervisor flag */
#define CPU_SRS_CARRY   BIT(2)  /* Carry flag */

/*
 * System memory
 *
 * @mem: Data
 * @mem_size: Memory size max
 */
struct sysmem {
    void *mem;
    size_t mem_size;
};

/*
 * CPU register state
 *
 * @xreg: X<n>
 * @ip: Instruction pointer
 * @sr_state: Processor state register
 * @blr: Branch link register
 * @ilr: Interrupt link register
 */
struct cpu_regs {
    reg_t xreg[16];
    reg_t ip;
    reg_t sr_state;
    reg_t blr;
    reg_t ilr;
};

struct oemu_cpu {
    struct cpu_regs regs;
};

void cpu_regdump(struct oemu_cpu *cpu);
void cpu_reset(struct oemu_cpu *cpu);
void cpu_kick(struct oemu_cpu *cpu, struct sysmem *mem);

#endif  /* !_OEMU_CPU_H_ */

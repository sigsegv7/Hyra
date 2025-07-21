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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <oemu/cpu.h>
#include <oemu/types.h>
#include <oemu/osmx64.h>

/*
 * Decode the INST_MOV_IMM instruction
 *
 * @cpu: CPU that is executing
 * @inst: Instruction dword
 */
static void
cpu_mov_imm(struct oemu_cpu *cpu, inst_t *inst)
{
    struct cpu_regs *regs = &cpu->regs;

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for 'mov'\n");
        return;
    }

    regs->xreg[inst->rd] = inst->imm;
    printf("#%d -> x%d\n", inst->imm, inst->rd);
}

/*
 * Decode the INST_INC instruction
 *
 * @cpu: CPU that is executing
 * @inst: Instruction dword
 */
static void
cpu_inc(struct oemu_cpu *cpu, inst_t *inst)
{
    struct cpu_regs *regs = &cpu->regs;
    imm_t imm;

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for 'mov'\n");
        return;
    }

    imm = regs->xreg[inst->rd]++;
    printf("INC X%d [%x], new=%x\n", inst->rd,
        imm, regs->xreg[inst->rd]);
}

/*
 * Decode the INST_DEC instruction
 *
 * @cpu: CPU that is executing
 * @inst: Instruction dword
 */
static void
cpu_dec(struct oemu_cpu *cpu, inst_t *inst)
{
    struct cpu_regs *regs = &cpu->regs;
    imm_t imm;

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for 'mov'\n");
        return;
    }

    imm = regs->xreg[inst->rd]--;
    printf("DEC X%d [%x], new=%x\n", inst->rd,
        imm, regs->xreg[inst->rd]);
}

/*
 * Decode the INST_ADD instruction
 *
 * @cpu: CPU that is executing
 * @inst: Instruction dword
 */
static void
cpu_add(struct oemu_cpu *cpu, inst_t *inst)
{
    struct cpu_regs *regs = &cpu->regs;
    imm_t imm;

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for 'add'\n");
        return;
    }

    imm = regs->xreg[inst->rd];
    regs->xreg[inst->rd] += inst->imm;
    printf("%d + %d -> X%d, new=%d\n",
        imm, inst->imm, inst->rd, regs->xreg[inst->rd]);
}

/*
 * Reset a CPU to a default state
 */
void
cpu_reset(struct oemu_cpu *cpu)
{
    struct cpu_regs *regs;

    regs = &cpu->regs;
    regs->ip = 0;
    memset(regs->xreg, 0x0, sizeof(regs->xreg));
}

/*
 * Main instruction execution loop.
 */
void
cpu_kick(struct oemu_cpu *cpu, struct sysmem *mem)
{
    struct cpu_regs *regs = &cpu->regs;
    inst_t *inst;
    uint8_t *memp = mem->mem;

    for (;;) {
        inst = (inst_t *)&memp[regs->ip];

        switch (inst->opcode) {
        case INST_MOV_IMM:
            cpu_mov_imm(cpu, inst);
            break;
        case INST_INC:
            cpu_inc(cpu, inst);
            break;
        case INST_DEC:
            cpu_dec(cpu, inst);
            break;
        case INST_ADD:
            cpu_add(cpu, inst);
            break;
        }

        /* Is this a halt instruction? */
        if (inst->opcode == INST_HLT) {
            printf("HALTED\n");
            break;
        }

        if (regs->ip >= MEMORY_SIZE) {
            break;
        }

        regs->ip += sizeof(*inst);
    }
}

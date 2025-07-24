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
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <oemu/cpu.h>
#include <oemu/types.h>
#include <oemu/osmx64.h>

/*
 * Return true if the instruction is an
 * MRO type instruction.
 */
static bool
cpu_is_mro(inst_t *inst)
{
    switch (inst->opcode) {
    case INST_MROB:
    case INST_MROW:
    case INST_MROD:
    case INST_MROQ:
        return true;
    }

    return false;
}

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
 * Decode the INST_SUB instruction
 *
 * @cpu: CPU that is executing
 * @inst: Instruction dword
 */
static void
cpu_sub(struct oemu_cpu *cpu, inst_t *inst)
{
    struct cpu_regs *regs = &cpu->regs;
    imm_t imm;

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for 'sub'\n");
        return;
    }

    imm = regs->xreg[inst->rd];
    regs->xreg[inst->rd] -= inst->imm;
    printf("%d - %d -> X%d, new=%d\n",
        imm, inst->imm, inst->rd, regs->xreg[inst->rd]);
}

/*
 * Decode the INST_MUL instruction
 *
 * @cpu: CPU that is executing
 * @inst: Instruction dword
 */
static void
cpu_mul(struct oemu_cpu *cpu, inst_t *inst)
{
    struct cpu_regs *regs = &cpu->regs;
    imm_t imm;

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for 'mul'\n");
        return;
    }

    imm = regs->xreg[inst->rd];
    regs->xreg[inst->rd] *= inst->imm;
    printf("%d * %d -> X%d, new=%d\n",
        imm, inst->imm, inst->rd, regs->xreg[inst->rd]);
}

/*
 * Decode the INST_DIV instruction
 *
 * @cpu: CPU that is executing
 * @inst: Instruction dword
 */
static void
cpu_div(struct oemu_cpu *cpu, inst_t *inst)
{
    struct cpu_regs *regs = &cpu->regs;
    imm_t imm;

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for 'div'\n");
        return;
    }

    imm = regs->xreg[inst->rd];
    if (imm == 0) {
        /* TODO: Some sort of interrupt */
        printf("** DIVIDE BY ZERO **\n");
        return;
    }

    regs->xreg[inst->rd] /= inst->imm;
    printf("%d / %d -> X%d, new=%d\n",
        imm, inst->imm, inst->rd, regs->xreg[inst->rd]);
}

/*
 * Decode the INST_DIV instruction
 */
static void
cpu_br(struct oemu_cpu *cpu, inst_t *inst)
{
    struct cpu_regs *regs = &cpu->regs;
    imm_t imm;
    addr_t br_to;

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for 'br'\n");
        return;
    }

    /*
     * If we are branching to the reset vector, might
     * as well reset all state.
     */
    br_to = regs->xreg[inst->rd];
    if (br_to == 0) {
        cpu_reset(cpu);
    }

    regs->ip = br_to;
}

/*
 * Decode MRO type instructions
 */
static void
cpu_mro(struct oemu_cpu *cpu, inst_t *inst)
{
    inst_t *next_inst;
    struct cpu_regs *regs = &cpu->regs;
    char *inst_str = "bad";
    uint64_t mask = 0;
    bool set_mask = false;
    imm_t imm;

    switch (inst->imm) {
    case 0: break;
    case 1:
        set_mask = true;
        break;
    default:
        imm = inst->imm & 1;
        if (inst->imm == 1) {
            set_mask = true;
        }
        break;
    }

    switch (inst->opcode) {
    case INST_MROB:
        inst_str = "mrob";
        if (!set_mask) {
            break;
        }
        mask |= MASK(8);
        break;
    case INST_MROW:
        inst_str = "mrow";
        if (!set_mask) {
            break;
        }
        mask |= MASK(16);
        break;
    case INST_MROD:
        inst_str = "mrod";
        if (!set_mask) {
            break;
        }
        mask |= MASK(32);
        break;
    case INST_MROQ:
        inst_str = "mroq";
        if (!set_mask) {
            break;
        }
        mask |= __UINT64_MAX;
        break;
    }

    if (inst->rd > NELEM(regs->xreg)) {
        printf("bad register operand for '%s'\n", inst_str);
        return;
    }

    if (set_mask) {
        imm = regs->xreg[inst->rd] |= mask;
        printf("set %x->x%d, new=%x\n", mask, inst->rd, imm);
    } else {
        imm = regs->xreg[inst->rd] &= ~mask;
        printf("cleared %x->x%d, new=%x\n", mask, inst->rd, imm);
    }
}

/*
 * Reset a CPU to a default state
 */
void
cpu_reset(struct oemu_cpu *cpu)
{
    struct cpu_regs *regs;

    /*
     * When an OSMX64 processor first starts up, it will
     * initially be executing in supervisor mode with all
     * of its registeres initialized to zeros.
     */
    regs = &cpu->regs;
    regs->ip = 0;
    regs->sr_state = CPU_SRS_SV;
    regs->blr = 0x0;
    regs->ilr = 0x0;
    memset(regs->xreg, 0x0, sizeof(regs->xreg));
}

void
cpu_regdump(struct oemu_cpu *cpu)
{
    struct cpu_regs *regs;

    regs = &cpu->regs;
    printf(
        "X0=%p, X1=%p, X2=%p\n"
        "X3=%p, X4=%p, X5=%p\n"
        "X6=%p, X7=%p, X8=%p\n"
        "X9=%p, X10=%p, X11=%p\n"
        "X12=%p, X13=%p, X14=%p\n"
        "X15=%p, IP=%p,  SRS=%p\n"
        "BLR=%p, ILR=%p\n",
        regs->xreg[0], regs->xreg[1],
        regs->xreg[2], regs->xreg[3],
        regs->xreg[4], regs->xreg[5],
        regs->xreg[6], regs->xreg[7],
        regs->xreg[8], regs->xreg[9],
        regs->xreg[10], regs->xreg[11],
        regs->xreg[12], regs->xreg[13],
        regs->xreg[14], regs->xreg[15],
        regs->ip, regs->sr_state,
        regs->blr, regs->ilr
    );
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
        case INST_NOP:
            /* NOP */
            regs->ip += sizeof(*inst);
            continue;
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
        case INST_SUB:
            cpu_sub(cpu, inst);
            break;
        case INST_MUL:
            cpu_mul(cpu, inst);
            break;
        case INST_DIV:
            cpu_div(cpu, inst);
            break;
        case INST_BR:
            cpu_br(cpu, inst);
            break;
        default:
            if (cpu_is_mro(inst)) {
                cpu_mro(cpu, inst);
            }
            break;
        }

        /*
         * X0 is readonly and should always be zero, undo
         * any writes or side effects from any operations
         * upon this register.
         */
        regs->xreg[0] = 0;

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

    cpu_regdump(cpu);
}

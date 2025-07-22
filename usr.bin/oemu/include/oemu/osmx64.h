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

#ifndef _OEMU_OSMX64_H_
#define _OEMU_OSMX64_H_

#include <stdint.h>

/* Opcodes */
#define INST_NOP      0x00    /* No-operation */
#define INST_ADD      0x01    /* Add operation */
#define INST_SUB      0x02    /* Sub operation */
#define INST_MUL      0x03    /* Multiply operation */
#define INST_DIV      0x04    /* Divide operation */
#define INST_INC      0x05    /* Increment operation */
#define INST_DEC      0x06    /* Decrement operation */
#define INST_OR       0x07    /* Bitwise OR operation */
#define INST_XOR      0x08    /* Bitwise XOR operation */
#define INST_AND      0x09    /* Bitwise AND operation */
#define INST_NOT      0x10    /* Bitwise NOT operation */
#define INST_SLL      0x11    /* Shift left logical operation */
#define INST_SRL      0x12    /* Shift right logical operation */
#define INST_MOV_IMM  0x13    /* Data move operation from IMM */
#define INST_HLT      0x14    /* Halt */
#define INST_BR       0x15    /* Branch */

/* Registers */
#define REG_X0   0x00
#define REG_X1   0x01
#define REG_X2   0x02
#define REG_X3   0x03
#define REG_X4   0x04
#define REG_X5   0x05
#define REG_X6   0x06
#define REG_X7   0x07
#define REG_X8   0x08
#define REG_X9   0x09
#define REG_X10  0x0A
#define REG_X11  0x0B
#define REG_X12  0x0C
#define REG_X13  0x0D
#define REG_X14  0x0E
#define REG_X15  0x0F
#define REG_BAD  0xFF

/*
 * OSMX64 instruction format
 */
typedef struct {
    uint8_t opcode;
    uint8_t rd;
    union {
        uint16_t imm;
        uint16_t unused;
    };
} inst_t;

#endif  /* !_OEMU_OSMX64_H_ */

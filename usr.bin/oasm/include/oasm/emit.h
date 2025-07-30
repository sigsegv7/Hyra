/* Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
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

#ifndef _EMIT_H_
#define _EMIT_H_

#include <sys/queue.h>
#include <stdint.h>
#include <stddef.h>
#include <oasm/lex.h>
#include <oasm/state.h>

/*
 * The OSMX64 architecture has 32-bit instructions
 * that are encoded in the following manner:
 *
 * - [0:7]: Opcode
 * - [11:8]: Register
 * - [31:12]: Reserved
 *
 * The values below define various operation
 * codes.
 */
#define OSMX64_NOP      0x00    /* No-operation */
#define OSMX64_ADD      0x01    /* Add operation */
#define OSMX64_SUB      0x02    /* Sub operation */
#define OSMX64_MUL      0x03    /* Multiply operation */
#define OSMX64_DIV      0x04    /* Divide operation */
#define OSMX64_INC      0x05    /* Increment operation */
#define OSMX64_DEC      0x06    /* Decrement operation */
#define OSMX64_OR       0x07    /* Bitwise OR operation */
#define OSMX64_XOR      0x08    /* Bitwise XOR operation */
#define OSMX64_AND      0x09    /* Bitwise AND operation */
#define OSMX64_NOT      0x0A    /* Bitwise NOT operation */
#define OSMX64_SLL      0x0B    /* Shift left logical operation */
#define OSMX64_SRL      0x0C    /* Shift right logical operation */
#define OSMX64_MOV_IMM  0x0D    /* Data move operation from IMM */
#define OSMX64_HLT      0x0E    /* Halt the processor */
#define OSMX64_BR       0x0F    /* Branch */
#define OSMX64_MROB     0x10    /* Mask register over byte */
#define OSMX64_MROW     0x11    /* Mask register over word */
#define OSMX64_MROD     0x12    /* Mask register over dword */
#define OSMX64_MROQ     0x13    /* Mask register over qword */
#define OSMX64_LSR      0x14    /* Logical shift right */
#define OSMX64_LSL      0x15    /* Logical shift left */

/*
 * OSMX64 register definitions
 */
#define OSMX64_R_X0   0x00
#define OSMX64_R_X1   0x01
#define OSMX64_R_X2   0x02
#define OSMX64_R_X3   0x03
#define OSMX64_R_X4   0x04
#define OSMX64_R_X5   0x05
#define OSMX64_R_X6   0x06
#define OSMX64_R_X7   0x07
#define OSMX64_R_X8   0x08
#define OSMX64_R_X9   0x09
#define OSMX64_R_X10  0x0A
#define OSMX64_R_X11  0x0B
#define OSMX64_R_X12  0x0C
#define OSMX64_R_X13  0x0D
#define OSMX64_R_X14  0x0E
#define OSMX64_R_X15  0x0F
#define OSMX64_R_BAD  0xFF

typedef uint8_t reg_t;
typedef uint16_t imm_t;

/*
 * OSMX64 instruction
 */
typedef struct {
    uint8_t opcode;
    uint8_t rd;
    union {
        uint16_t imm;
        uint16_t unused;
    };
} inst_t;

struct emit_state {
    tt_t last_token;
    uint8_t is_init : 1;
    int out_fd;
    TAILQ_HEAD(, oasm_token) ir;
};

int emit_init(struct emit_state *state);
int emit_destroy(struct emit_state *state);
int emit_process(struct oasm_state *oasm, struct emit_state *emit);
int emit_osmx64(struct emit_state *state, struct oasm_token *tp);

#endif  /* !_EMIT_H_ */

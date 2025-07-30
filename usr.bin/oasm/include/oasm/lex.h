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

#ifndef _OASM_LEX_H_
#define _OASM_LEX_H_

#include <sys/queue.h>
#include <sys/cdefs.h>
#include <stdint.h>
#include <stdbool.h>

struct oasm_state;

#define __XN_REGS   \
    TT_X0,          \
    TT_X1,          \
    TT_X2,          \
    TT_X3,          \
    TT_X4,          \
    TT_X5,          \
    TT_X6,          \
    TT_X7,          \
    TT_X8,          \
    TT_X9,          \
    TT_X10,         \
    TT_X11,         \
    TT_X12,         \
    TT_X13,         \
    TT_X14,         \
    TT_X15

#define __FN_REGS   \
    TT_F0,          \
    TT_F1,          \
    TT_F2,          \
    TT_F3,          \
    TT_F4,          \
    TT_F5,          \
    TT_F6,          \
    TT_F7

#define __DN_REGS   \
    TT_D0,          \
    TT_D1,          \
    TT_D2,          \
    TT_D3,          \
    TT_D4,          \
    TT_D5,          \
    TT_D6,          \
    TT_D7

#define __VN_REGS   \
    TT_V0,          \
    TT_V1,          \
    TT_V2,          \
    TT_V3,          \
    TT_V4,          \
    TT_V5,          \
    TT_V6,          \
    TT_V7

/*
 * Token type definitions
 */
typedef enum {
    TT_UNKNOWN,     /* Unknown token */
    TT_NOP,         /* No operation */

    /* Arithmetic instructions */
    TT_ADD,         /* 'add' */
    TT_SUB,         /* 'sub' */
    TT_MUL,         /* 'mul' */
    TT_DIV,         /* 'div' */
    TT_HLT,         /* 'hlt' */
    TT_BR,          /* 'br'  */
    TT_MROB,        /* 'mrob' */
    TT_MROW,        /* 'mrow' */
    TT_MROD,        /* 'mrod' */
    TT_MROQ,        /* 'mroq' */
    TT_AND,         /* 'and' */
    TT_OR,          /* 'or' */
    TT_XOR,         /* 'xor' */
    TT_LSR,         /* 'lsr' */
    TT_LSL,         /* 'lsl' */

    /* Register ops */
    TT_MOV,         /* 'mov' */
    TT_INC,         /* 'inc' */
    TT_DEC,         /* 'dec' */
    TT_IMM,         /* #<n> */
    TT_LABEL,       /* 'label: ...' */

    /* Register sets */
    __XN_REGS,      /* x0-x15 */
    __FN_REGS,      /* f0-f7 */
    __DN_REGS,      /* d0-d7 */
    __VN_REGS,      /* v0-v7 */

    /* Symbols */
    TT_COMMA,       /* ',' */
} tt_t;

struct oasm_token {
    tt_t type;
    uint8_t is_reg : 1;
    uint16_t imm;
    char *raw;
    TAILQ_ENTRY(oasm_token) link;
};

int lex_tok(struct oasm_state *state, struct oasm_token *ttp);


/*
 * Check if a token is an X<n> register.
 * Returns true on match.
 */
__always_inline static inline bool
tok_is_xreg(tt_t tok)
{
    switch (tok) {
    case TT_X0:
    case TT_X1:
    case TT_X2:
    case TT_X3:
    case TT_X4:
    case TT_X5:
    case TT_X6:
    case TT_X7:
    case TT_X8:
    case TT_X9:
    case TT_X10:
    case TT_X11:
    case TT_X12:
    case TT_X13:
    case TT_X14:
    case TT_X15:
        return true;
    }

    return false;
}

/*
 * Check if a token is of an MRO type
 * instruction. Returns true on match.
 */
__always_inline static inline bool
tok_is_mro(tt_t tok)
{
    switch (tok) {
    case TT_MROB:
    case TT_MROW:
    case TT_MROD:
    case TT_MROQ:
        return true;
    }

    return false;
}

#endif  /* !_OASM_LEX_H_ */

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

#include <sys/errno.h>
#include <oasm/emit.h>
#include <oasm/log.h>
#include <stdlib.h>
#include <string.h>

static inline void
emit_bytes(struct emit_state *state, void *p, size_t len)
{
    write(state->out_fd, p, len);
}

/*
 * Convert an IR register to an OSMX64
 * valid register value that can be encoded
 * into the instruction.
 */
static inline reg_t
ir_to_reg(tt_t ir)
{
    switch (ir) {
        case TT_X0: return OSMX64_R_X0;
        case TT_X1: return OSMX64_R_X1;
        case TT_X2: return OSMX64_R_X2;
        case TT_X3: return OSMX64_R_X3;
        case TT_X4: return OSMX64_R_X4;
        case TT_X5: return OSMX64_R_X5;
        case TT_X6: return OSMX64_R_X6;
        case TT_X7: return OSMX64_R_X7;
        case TT_X8: return OSMX64_R_X8;
        case TT_X9: return OSMX64_R_X9;
        case TT_X10: return OSMX64_R_X10;
        case TT_X11: return OSMX64_R_X11;
        case TT_X12: return OSMX64_R_X12;
        case TT_X13: return OSMX64_R_X13;
        case TT_X14: return OSMX64_R_X14;
        case TT_X15: return OSMX64_R_X15;
    }

    return OSMX64_R_BAD;
}

/*
 * Encode a MOV instruction
 *
 * mov [r], [r/imm]
 *
 * Returns the next token on success,
 * otherwise NULL.
 */
static struct oasm_token *
emit_encode_mov(struct emit_state *state, struct oasm_token *tok)
{
    inst_t curinst;
    reg_t rd;

    if (state == NULL || tok == NULL) {
        return NULL;
    }

    /* Next token should be a register */
    tok = TAILQ_NEXT(tok, link);
    if (tok == NULL) {
        return NULL;
    }
    if (!tok_is_xreg(tok->type)) {
        oasm_err("[emit error]: bad 'mov' order\n");
        return NULL;
    }

    rd = ir_to_reg(tok->type);
    if (rd == OSMX64_R_BAD) {
        oasm_err("[emit error]: got bad reg in 'mov'\n");
        return NULL;
    }

    /* Next token should be an IMM */
    tok = TAILQ_NEXT(tok, link);
    if (tok == NULL) {
        oasm_err("[emit error]: bad 'mov' order\n");
        return NULL;
    }
    if (tok->type != TT_IMM) {
        oasm_err("[emit error]: expected <imm>\n");
        return NULL;
    }

    curinst.opcode = OSMX64_MOV_IMM;
    curinst.rd = rd;
    curinst.imm = tok->imm;
    emit_bytes(state, &curinst, sizeof(curinst));
    return TAILQ_NEXT(tok, link);
}

/*
 * Encode a INC/DEC instruction
 *
 * inc/dec [r]
 *
 * Returns the next token on success,
 * otherwise NULL.
 */
static struct oasm_token *
emit_encode_incdec(struct emit_state *state, struct oasm_token *tok)
{
    inst_t curinst;
    reg_t rd;
    uint8_t opcode = OSMX64_INC;
    char *inst_str = "inc";

    if (state == NULL || tok == NULL) {
        return NULL;
    }

    if (tok->type == TT_DEC) {
        inst_str = "dec";
        opcode = OSMX64_DEC;
    }

    /* Next token should be a register */
    tok = TAILQ_NEXT(tok, link);
    if (tok == NULL) {
        return NULL;
    }
    if (!tok_is_xreg(tok->type)) {
        oasm_err("[emit error]: bad '%s' order\n", inst_str);
        return NULL;
    }

    rd = ir_to_reg(tok->type);
    if (rd == OSMX64_R_BAD) {
        oasm_err("[emit error]: got bad reg in '%s'\n", inst_str);
        return NULL;
    }

    curinst.opcode = opcode;
    curinst.rd = rd;
    curinst.unused = 0;
    emit_bytes(state, &curinst, sizeof(curinst));
    return TAILQ_NEXT(tok, link);
}

/*
 * Encode an arithmetic instruction
 *
 * add [r], <imm>
 *
 * Returns the next token on success,
 * otherwise NULL.
 */
static struct oasm_token *
emit_encode_arith(struct emit_state *state, struct oasm_token *tok)
{
    inst_t curinst;
    reg_t rd;
    uint8_t opcode = OSMX64_ADD;
    char *inst_str = "add";

    switch (tok->type) {
    case TT_SUB:
        inst_str = "sub";
        opcode = OSMX64_SUB;
        break;
    case TT_MUL:
        inst_str = "mul";
        opcode = OSMX64_MUL;
        break;
    case TT_DIV:
        inst_str = "div";
        opcode = OSMX64_DIV;
        break;
    }

    /*
     * The next operand must be an X<n>
     * register.
     */
    tok = TAILQ_NEXT(tok, link);
    if (tok == NULL) {
        return NULL;
    }
    if (!tok_is_xreg(tok->type)) {
        oasm_err("[emit error]: bad '%s' order\n", inst_str);
        return NULL;
    }

    /* Get the register and validate it */
    rd = ir_to_reg(tok->type);
    if (rd == OSMX64_R_BAD) {
        oasm_err("[emit error]: got bad reg in '%s'\n", inst_str);
        return NULL;
    }

    /* The next token should be an <imm> */
    tok = TAILQ_NEXT(tok, link);
    if (tok == NULL) {
        return NULL;
    }
    if (tok->type != TT_IMM) {
        oasm_err("[emit error]: expected <imm> in '%s'\n", inst_str);
        return NULL;
    }

    curinst.opcode = opcode;
    curinst.rd = rd;
    curinst.imm = tok->imm;
    emit_bytes(state, &curinst, sizeof(curinst));
    return TAILQ_NEXT(tok, link);
}

/*
 * Encode a HLT instruction
 *
 * 'hlt' - no operands
 *
 * Returns the next token on success,
 * otherwise NULL.
 */
static struct oasm_token *
emit_encode_hlt(struct emit_state *state, struct oasm_token *tok)
{
    inst_t curinst;

    curinst.opcode = OSMX64_HLT;
    curinst.rd = 0;
    curinst.unused = 0;
    emit_bytes(state, &curinst, sizeof(curinst));
    return TAILQ_NEXT(tok, link);
}

/*
 * Encode a BR instruction
 *
 * br [r]
 *
 * Returns the next token on success,
 * otherwise NULL.
 */
static struct oasm_token *
emit_encode_br(struct emit_state *state, struct oasm_token *tok)
{
    inst_t curinst;
    reg_t rd;
    uint8_t opcode = OSMX64_BR;
    char *inst_str = "br";

    /* Grab the register */
    tok = TAILQ_NEXT(tok, link);
    if (tok == NULL) {
        return NULL;
    }
    if (!tok_is_xreg(tok->type)) {
        oasm_err("[emit error]: expected register in '%s'\n", inst_str);
        return NULL;
    }

    rd = ir_to_reg(tok->type);
    if (rd == OSMX64_R_BAD) {
        oasm_err("[emit error]: got bad in register in '%s'\n", inst_str);
        return NULL;
    }

    curinst.opcode = opcode;
    curinst.rd = rd;
    curinst.unused = 0;
    emit_bytes(state, &curinst, sizeof(curinst));
    return TAILQ_NEXT(tok, link);
}

/*
 * Encode the MRO type instructions
 *
 * mrob x1[7:0]
 * mrow x1[15:0]   ! Mrowwww :3333
 * mrod x1[31:0]
 * mroq x[63:0]
 *
 * Returns the next token on success,
 * otherwise NULL.
 */
static struct oasm_token *
emit_encode_mro(struct emit_state *state, struct oasm_token *tok)
{
    inst_t curinst;
    reg_t rd;
    uint8_t opcode = OSMX64_MROB;
    char *inst_str = "mrob";

    switch (tok->type) {
    case TT_MROW:
        opcode = OSMX64_MROW;
        inst_str = "mrow";
        break;
    case TT_MROD:
        opcode = OSMX64_MROD;
        inst_str = "mrod";
        break;
    case TT_MROQ:
        opcode = OSMX64_MROQ;
        inst_str = "mroq";
        break;
    }

    /* Next token should be a register */
    tok = TAILQ_NEXT(tok, link);
    if (!tok_is_xreg(tok->type)) {
        oasm_err("[emit error]: expected register in '%s'\n", inst_str);
        return NULL;
    }

    rd = ir_to_reg(tok->type);
    if (rd == OSMX64_R_BAD) {
        oasm_err("[emit error]: got bad register in '%s'\n", inst_str);
        return NULL;
    }

    /* Next token should be an IMM */
    tok = TAILQ_NEXT(tok, link);
    if (tok->type != TT_IMM) {
        oasm_err("[emit error]: expected <imm> after reg in '%s'\n", inst_str);
        return NULL;
    }

    curinst.opcode = opcode;
    curinst.rd = rd;
    curinst.imm = tok->imm;
    emit_bytes(state, &curinst, sizeof(curinst));
    return TAILQ_NEXT(tok, link);
}

/*
 * Encode a NOP instruction
 *
 * 'nop' - no operands
 *
 * Returns the next token on success,
 * otherwise NULL.
 */
static struct oasm_token *
emit_encode_nop(struct emit_state *state, struct oasm_token *tok)
{
    inst_t curinst;

    curinst.opcode = OSMX64_NOP;
    curinst.rd = 0;
    curinst.unused = 0;
    emit_bytes(state, &curinst, sizeof(curinst));
    return TAILQ_NEXT(tok, link);
}

/*
 * Encode a bitwise instruction:
 *
 * and r, r/imm
 * or r, r/imm
 * xor r, r/imm
 */
static struct oasm_token *
emit_encode_bitw(struct emit_state *state, struct oasm_token *tok)
{
    inst_t curinst;
    imm_t imm;
    reg_t rd;
    uint8_t opcode = OSMX64_AND;
    char *inst_str = "and";

    switch (tok->type) {
    case TT_OR:
        opcode = OSMX64_OR;
        inst_str = "or";
        break;
    case TT_XOR:
        opcode = OSMX64_XOR;
        inst_str = "xor";
        break;
    case TT_LSR:
        opcode = OSMX64_LSR;
        inst_str = "lsr";
        break;
    case TT_LSL:
        opcode = OSMX64_LSL;
        inst_str = "lsl";
        break;
    }

    /* Next token should be a register */
    tok = TAILQ_NEXT(tok, link);
    if (tok == NULL) {
        oasm_err("[emit error]: expected register for '%s'\n", inst_str);
        return NULL;
    }
    if (!tok_is_xreg(tok->type)) {
        oasm_err("[emit error]: bad register for '%s'\n", inst_str);
        return NULL;
    }

    rd = ir_to_reg(tok->type);
    tok = TAILQ_NEXT(tok, link);
    if (tok == NULL) {
        oasm_err("[emit error]: missing operand in '%s'\n", inst_str);
        return NULL;
    }

    /*
     * Check that the next token is an immediate
     * value.
     *
     * TODO: Allow a register operand to be passed
     *       to these instructions.
     */
    if (tok->type != TT_IMM) {
        oasm_err("[emit error]: expected <imm> for '%s'\n", inst_str);
        return NULL;
    }

    imm = tok->imm;
    curinst.opcode = opcode;
    curinst.rd = rd;
    curinst.imm = imm;
    emit_bytes(state, &curinst, sizeof(curinst));
    return TAILQ_NEXT(tok, link);
}

int
emit_osmx64(struct emit_state *state, struct oasm_token *tp)
{
    struct oasm_token *toknew;

    if (state == NULL || tp == NULL) {
        return -EINVAL;
    }

    /*
     * We need to create a copy of the object as the
     * caller will likely end up destroying it.
     */
    toknew = malloc(sizeof(*toknew));
    if (toknew == NULL) {
        return -ENOMEM;
    }

    memcpy(toknew, tp, sizeof(*toknew));
    TAILQ_INSERT_TAIL(&state->ir, toknew, link);
    return 0;
}

int
emit_init(struct emit_state *state)
{
    state->last_token = TT_UNKNOWN;
    state->is_init = 1;
    TAILQ_INIT(&state->ir);
    return 0;
}

int
emit_destroy(struct emit_state *state)
{
    struct oasm_token *curtok, *last = NULL;

    TAILQ_FOREACH(curtok, &state->ir, link) {
        if (last != NULL) {
            free(last);
            last = NULL;
        }
        if (curtok->raw != NULL) {
            free(curtok->raw);
        }

        last = curtok;
    }

    /* Clean up any last objects */
    if (last != NULL) {
        free(last);
    }

    return 0;
}

int
emit_process(struct oasm_state *oasm, struct emit_state *emit)
{
    struct oasm_token *curtok;
    tt_t last_tok;

    if (!emit->is_init) {
        return -1;
    }

    emit->out_fd = oasm->out_fd;
    curtok = TAILQ_FIRST(&emit->ir);
    while (curtok != NULL) {
        switch (curtok->type) {
        case TT_NOP:
            curtok = emit_encode_nop(emit, curtok);
            break;
        case TT_MOV:
            curtok = emit_encode_mov(emit, curtok);
            break;
        case TT_INC:
        case TT_DEC:
            curtok = emit_encode_incdec(emit, curtok);
            break;
        case TT_ADD:
        case TT_SUB:
        case TT_MUL:
        case TT_DIV:
            curtok = emit_encode_arith(emit, curtok);
            break;
        case TT_AND:
        case TT_OR:
        case TT_XOR:
        case TT_LSR:
        case TT_LSL:
            curtok = emit_encode_bitw(emit, curtok);
            break;
        case TT_BR:
            curtok = emit_encode_br(emit, curtok);
            break;
        case TT_HLT:
            curtok = emit_encode_hlt(emit, curtok);
            break;
        default:
            if (tok_is_mro(curtok->type)) {
                curtok = emit_encode_mro(emit, curtok);
                break;
            }
            curtok = TAILQ_NEXT(curtok, link);
            break;
        }
    }

    return 0;
}

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

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <oasm/emit.h>
#include <oasm/state.h>
#include <oasm/lex.h>
#include <oasm/parse.h>
#include <oasm/log.h>

static struct emit_state emit_state;
static const char *tokstr[] = {
    [ TT_UNKNOWN] = "bad",
    [ TT_ADD ] = "add",
    [ TT_SUB ] = "sub",
    [ TT_MUL ] = "mul",
    [ TT_DIV ] = "div",
    [ TT_HLT ] = "hlt",
    [ TT_COMMA ] = ",",
    [ TT_INC ] = "inc",
    [ TT_DEC ] = "dec",
    [ TT_MOV ] = "mov",
    [ TT_IMM ]  = "<imm>",

    /* X<n> registers */
    [ TT_X0 ]  = "x0",
    [ TT_X1 ]  = "x1",
    [ TT_X2 ]  = "x2",
    [ TT_X3 ]  = "x3",
    [ TT_X4 ]  = "x4",
    [ TT_X5 ]  = "x5",
    [ TT_X6 ]  = "x6",
    [ TT_X7  ] = "x7",
    [ TT_X8 ]  = "x8",
    [ TT_X9 ]  = "x9",
    [ TT_X10 ] = "x10",
    [ TT_X11 ] = "x11",
    [ TT_X12 ] = "x12",
    [ TT_X13 ] = "x13",
    [ TT_X14 ] = "x14",
    [ TT_X15 ] = "x15",

    /* V<n> registers */
    [ TT_F0 ]  = "v0",
    [ TT_F1 ]  = "v1",
    [ TT_F2 ]  = "v2",
    [ TT_F3 ]  = "v3",
    [ TT_F4 ]  = "v4",
    [ TT_F5 ]  = "v5",
    [ TT_F6 ]  = "v6",
    [ TT_F7 ]  = "v7",

    /* D<n> registers */
    [ TT_D0 ]  = "d0",
    [ TT_D1 ]  = "d1",
    [ TT_D2 ]  = "d2",
    [ TT_D3 ]  = "d3",
    [ TT_D4 ]  = "d4",
    [ TT_D5 ]  = "d5",
    [ TT_D6 ]  = "d6",
    [ TT_D7 ]  = "d7",

    /* V<n> registers */
    [ TT_V0 ]  = "v0",
    [ TT_V1 ]  = "v1",
    [ TT_V2 ]  = "v2",
    [ TT_V3 ]  = "v3",
    [ TT_V4 ]  = "v4",
    [ TT_V5 ]  = "v5",
    [ TT_V6 ]  = "v6",
    [ TT_V7 ]  = "v7",
};

static int
parse_reg(struct oasm_state *state, struct oasm_token *tok)
{
    const char *p;

    /* Valid instructions that go with regs */
    switch (state->last) {
    case TT_MOV:
    case TT_DEC:
    case TT_INC:
    case TT_ADD:
    case TT_SUB:
        state->last = tok->type;
        break;
    default:
        p = tokstr[state->last];
        oasm_err("bad instruction '%s' for regop\n", p);
        return -1;
    }

    if (!tok_is_xreg(tok->type)) {
        p = tokstr[tok->type];
        oasm_err("bad register \"%s\"\n", p);
        return -1;
    }

    state->last = tok->type;
    emit_osmx64(&emit_state, tok);
    return 0;
}

static int
parse_imm(struct oasm_token *tok, tt_t last)
{
    return 0;
}

static int
parse_tok(struct oasm_state *state, struct oasm_token *tok)
{
    const char *p;
    int error;

    switch (tok->type) {
    case TT_HLT:
        state->last = tok->type;
        emit_osmx64(&emit_state, tok);
        break;
    case TT_MOV:
        state->last = tok->type;
        emit_osmx64(&emit_state, tok);
        break;
    case TT_ADD:
        state->last = tok->type;
        emit_osmx64(&emit_state, tok);
        break;
    case TT_SUB:
        state->last = tok->type;
        emit_osmx64(&emit_state, tok);
        break;
    case TT_DEC:
    case TT_INC:
        state->last = tok->type;
        emit_osmx64(&emit_state, tok);
        break;
    case TT_IMM:
        p = tokstr[state->last];
        if (!tok_is_xreg(state->last)) {
            printf("expected X<n> but got %s\n", p);
            return -1;
        }
        emit_osmx64(&emit_state, tok);
        break;
    default:
        if (!tok->is_reg) {
            oasm_err("syntax error\n");
            return -1;
        }

        error = parse_reg(state, tok);
        if (error < 0) {
            return error;
        }
        break;
    }

    return 0;
}

void
parse_enter(struct oasm_state *state)
{
    struct oasm_token tok;
    const char *type, *raw;
    int error = 0;

    emit_init(&emit_state);

    for (;;) {
        error = lex_tok(state, &tok);
        if (error < 0) {
            break;
        }

        if (parse_tok(state, &tok) < 0) {
            break;
        }

        type = tokstr[tok.type];
        raw = tok.raw;
        oasm_debug("got token type %s (%s)\n", type, raw);
    }

    /* Process then destroy the emit state */
    emit_process(state, &emit_state);
    emit_destroy(&emit_state);
}

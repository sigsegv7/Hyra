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
#include <string.h>
#include <stdlib.h>
#include <oasm/state.h>
#include <oasm/lex.h>
#include <oasm/log.h>

#define is_num(c) ((c) >= '0' && (c) <= '9')

static char putback = '\0';

/* Instruction mnemonic strings */
#define S_IMN_MOV  "mov"
#define S_IMN_ADD  "add"
#define S_IMN_SUB  "sub"
#define S_IMN_MUL  "mul"
#define S_IMN_DIV  "div"
#define S_IMN_INC  "inc"
#define S_IMN_DEC  "dec"
#define S_IMN_HLT  "hlt"
#define S_IMN_BR   "br"

/*
 * Returns 0 if a char is counted as a
 * skippable token. Otherwise, -1
 */
static inline int
lex_skippable(struct oasm_state *state, char c)
{
    switch (c) {
    case ' ':  return 0;
    case '\f': return 0;
    case '\t': return 0;
    case '\r': return 0;
    case '\n':
        ++state->line;
        return 0;
    }

    return -1;
}

/*
 * For cleaning up allocated sources
 * during error conditions
 *
 * @p: Memory to free
 */
static inline void
lex_try_free(void *p)
{
    if (p != NULL) {
        free(p);
    }
}

/*
 * Put back a token to grab later
 *
 * @c: Character to put back
 */
static inline char
lex_putback(char c)
{
    putback = c;
    return c;
}

/*
 * Grab a character from the input file
 * descriptor.
 */
static char
lex_cin(struct oasm_state *state)
{
    char retval;

    if (putback != '\0') {
        retval = putback;
        putback = '\0';
        return retval;
    }

    if (read(state->in_fd, &retval, 1) <= 0) {
        return '\0';
    }
    return retval;
}

/*
 * Nom an operation, directive or any kind
 * of raw string (unquoted/builtin) and return
 * memory allocated by strdup() pointing to the
 * string.
 *
 * @state: OASM state pointer
 * @res: Resulting string
 *
 * Returns 0 on success. Greater than zero
 * value of the last character if a comma or
 * space was not buffered.
 */
static int
lex_nomstr(struct oasm_state *state, char **res)
{
    char buf[256];
    int retval = 0, n = 0;
    int tmp;

    memset(buf, 0, sizeof(buf));

    /*
     * We are filling the buffer containing
     * the operation or directive.
     *
     * Keep going until we hit a space or comman (^)
     * Examples of such strings (everything in '[]'):
     *
     * [mov] [x0], [#1]
     *      ^    ^
     */
    while ((tmp = lex_cin(state)) != 0) {
        if (tmp == ' ' || tmp == ',') {
            retval = tmp;
            break;
        }
        if (tmp == '\n') {
            ++state->line;
            retval = tmp;
            break;

        }

        buf[n++] = tmp;
    }

    *res = strdup(buf);
    return retval;
}

static tt_t
token_arith(char *p)
{
    if (strcmp(p, S_IMN_MOV) == 0) {
        return TT_MOV;
    } else if (strcmp(p, S_IMN_INC) == 0) {
        return TT_INC;
    } else if (strcmp(p, S_IMN_DEC) == 0) {
        return TT_DEC;
    } else if (strcmp(p, S_IMN_ADD) == 0) {
        return TT_ADD;
    } else if (strcmp(p, S_IMN_SUB) == 0) {
        return TT_SUB;
    } else if (strcmp(p, S_IMN_DIV) == 0) {
        return TT_DIV;
    } else if (strcmp(p, S_IMN_HLT) == 0) {
        return TT_HLT;
    } else if (strcmp(p, S_IMN_MUL) == 0) {
        return TT_MUL;
    }

    return TT_UNKNOWN;
}

/*
 * Control flow instructions
 */
static tt_t
token_cfi(char *p)
{
    if (strcmp(p, S_IMN_BR) == 0) {
        return TT_BR;
    }

    return TT_UNKNOWN;
}

static tt_t
token_xreg(char *p)
{
    int num;

    if (p[0] != 'x') {
        return TT_UNKNOWN;
    }

    if (!is_num(p[1])) {
        return TT_UNKNOWN;
    }

    num = atoi(&p[1]);
    switch (num) {
        case 0: return TT_X0;
        case 1: return TT_X1;
        case 2: return TT_X2;
        case 3: return TT_X3;
        case 4: return TT_X4;
        case 5: return TT_X5;
        case 6: return TT_X6;
        case 7: return TT_X7;
        case 8: return TT_X8;
        case 9: return TT_X9;
        case 10: return TT_X10;
        case 11: return TT_X11;
        case 12: return TT_X12;
        case 13: return TT_X13;
        case 14: return TT_X14;
        case 15: return TT_X15;
    }

    return TT_UNKNOWN;
}

static tt_t
token_operand(char *p)
{
    /* Is this a numeric constant? */
    if (p[0] == '#') {
        return TT_IMM;
    }

    return TT_UNKNOWN;
}

static tt_t
token_reg(char *p)
{
    tt_t tok;

    if ((tok = token_xreg(p)) != TT_UNKNOWN) {
        return tok;
    }

    return TT_UNKNOWN;
}

int
lex_tok(struct oasm_state *state, struct oasm_token *ttp)
{
    char *p = NULL;
    char c = ' ';
    int tmp;
    tt_t tok;

    if (state == NULL || ttp == NULL) {
        return -EINVAL;
    }

    /*
     * Grab characters. If they are skippable,
     * don't use them.
     */
    while (lex_skippable(state, c) == 0) {
        if ((c = lex_cin(state)) == 0) {
            return -1;
        }
    }

    switch (c) {
    case '\n':
        ++state->line;
        return 0;
    case '\0':
        return -1;
    case ',':
        return TT_COMMA;
    default:
        ttp->type = TT_UNKNOWN;
        ttp->raw = NULL;

        lex_putback(c);
        lex_nomstr(state, &p);

        /* Arithmetic operation? */
        if ((tok = token_arith(p)) != TT_UNKNOWN) {
            ttp->type = tok;
            ttp->raw = p;
            return 0;
        }

        /* Control flow instruction? */
        if ((tok = token_cfi(p)) != TT_UNKNOWN) {
            ttp->type = tok;
            ttp->raw = p;
            return 0;
        }

        /* Register? */
        if ((tok = token_reg(p)) != TT_UNKNOWN) {
            ttp->is_reg = 1;
            ttp->type = tok;
            ttp->raw = p;
            return 0;
        }

        /* Immediate operand? */
        if ((tok = token_operand(p)) != TT_UNKNOWN) {
            if (tok == TT_IMM) {
                ttp->imm = atoi(&p[1]);
            }

            ttp->type = tok;
            ttp->raw = p;
            return 0;
        }
        oasm_err("bad token \"%s\"\n", p);
        lex_try_free(p);
        return -1;
    }

    return 0;
}

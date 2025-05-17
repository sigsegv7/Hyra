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

#ifndef _DEV_CONSVAR_H_
#define _DEV_CONSVAR_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/spinlock.h>

/* Buffer types */
#define CONS_BUF_INPUT   0
#define CONS_BUF_OUTPUT  1

/* Buffer flags */
#define CONS_BUF_CLEAN   BIT(0)     /* Not recently written to */

extern struct cons_screen scr;

/*
 * The keyboard packet is two bytes
 * and the bits are as follows:
 *
 * - 0:7  ~ ASCII character
 * - 8:15 ~ Scancode
 */
struct cons_input {
    union {
        uint8_t chr;
        uint8_t scancode;
    };
    uint16_t data;
};

/*
 * A circular buffer for buffering
 * keyboard input or console output.
 */
struct cons_buf {
    struct spinlock lock;
    union {
        struct cons_input *ibuf;
        struct cons_char *obuf;
        void *raw;
    };
    uint8_t tail;
    uint8_t head;
    uint8_t type;
    uint8_t flags;
    size_t len;
};

struct cons_buf *cons_new_buf(uint8_t type, size_t len);
int cons_obuf_push(struct cons_buf *bp, struct cons_char c);
int cons_obuf_pop(struct cons_buf *bp, struct cons_char *res);

int cons_ibuf_push(struct cons_screen *scr, struct cons_input input);
int cons_ibuf_pop(struct cons_screen *scr, struct cons_input *res);

#endif  /* !_DEV_CONSVAR_H_ */

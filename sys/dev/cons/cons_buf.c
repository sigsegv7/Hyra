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
#include <sys/errno.h>
#include <dev/cons/consvar.h>
#include <dev/cons/cons.h>
#include <vm/dynalloc.h>
#include <string.h>
#include <assert.h>

/*
 * Create a new console buffer.
 *
 * @type: Buffer type (CONS_BUF_*)
 * @len: Max length of buffer.
 */
struct cons_buf *
cons_new_buf(uint8_t type, size_t len)
{
    struct cons_buf *bp;
    size_t alloc_len;

    if ((bp = dynalloc(sizeof(*bp))) == NULL) {
        return NULL;
    }

    memset(bp, 0, sizeof(*bp));
    bp->type = type;
    bp->len = len;

    /* Create the actual buffers now */
    switch (type) {
    case CONS_BUF_INPUT:
        alloc_len = sizeof(*bp->ibuf) * len;
        bp->ibuf = dynalloc(alloc_len);
        break;
    case CONS_BUF_OUTPUT:
        alloc_len = sizeof(*bp->obuf) * len;
        bp->obuf = dynalloc(alloc_len);
        break;
    }

    return bp;
}

/*
 * Push a character to a console output
 * buffer.
 *
 * @bp: Pointer to console buffer.
 * @c: Character to push.
 */
int
cons_obuf_push(struct cons_buf *bp, struct cons_char c)
{
    uint8_t next;

    if (bp == NULL) {
        return -EINVAL;
    }

    __assert(bp->type == CONS_BUF_OUTPUT);
    next = bp->head + 1;
    if (next > bp->len) {
        return -ENOSPC;
    }

    bp->obuf[bp->head] = c;
    bp->head = next;
    return 0;
}

/*
 * Pop a character from the console
 * buffer.
 *
 * @bp: Pointer to console buffer.
 * @res: Result is written here.
 */
int
cons_obuf_pop(struct cons_buf *bp, struct cons_char *res)
{
    uint8_t next;

    if (bp == NULL || res == NULL) {
        return -EINVAL;
    }

    __assert(bp->type == CONS_BUF_OUTPUT);

    /* Do we have any data left? */
    if (bp->head == bp->tail) {
        bp->head = 0;
        bp->tail = 0;
        return -EAGAIN;
    }

    next = bp->tail + 1;
    if (next > bp->len) {
        next = 0;
    }

    *res = bp->obuf[bp->tail];
    bp->tail = next;
    return 0;
}

int
cons_ibuf_push(struct cons_screen *scr, struct cons_input in)
{
    struct cons_buf *bp;
    uint8_t head_next;

    if (scr == NULL) {
        return -EINVAL;
    }

    bp = scr->ib;
    __assert(bp->type == CONS_BUF_INPUT);

    head_next = bp->head + 1;
    if (head_next > bp->len) {
        return -ENOSPC;
    }

    bp->ibuf[bp->head] = in;
    bp->head = head_next;
    return 0;
}

int
cons_ibuf_pop(struct cons_screen *scr, struct cons_input *res)
{
    uint8_t tail_next;
    struct cons_buf *bp;

    if (scr == NULL || res == NULL) {
        return -EINVAL;
    }

    bp = scr->ib;
    __assert(bp->type == CONS_BUF_INPUT);

    /* Do we have any data left? */
    if (bp->head == bp->tail) {
        bp->head = 0;
        bp->tail = 0;
        return -EAGAIN;
    }

    tail_next = bp->tail + 1;
    if (tail_next > bp->len) {
        tail_next = 0;
    }

    *res = bp->ibuf[bp->tail];
    bp->tail = tail_next;
    return 0;
}

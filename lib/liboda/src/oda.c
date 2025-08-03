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
#include <stdio.h>
#include <libgfx/gfx.h>
#include <liboda/oda.h>
#include <liboda/odavar.h>

#define oda_log(fmt, ...) printf("oda: " fmt, ##__VA_ARGS__)

/*
 * Initialize the OSMORA Display Architecture
 * (ODA) library.
 *
 * @res: Initialized ODA state result
 *
 * Returns 0 on success, otherwise a less than
 * zero value.
 */
int
oda_init(struct oda_state *res)
{
    int error;

    /* Ensure the argument is valid */
    if (res == NULL) {
        return -EINVAL;
    }

    /*
     * If this state has already been initialized,
     * assume programmer error / undefined behaviour
     * and let them know.
     */
    if (oda_cookie_verify(res) == 0) {
        oda_log("oda_init: 'res' already initialized\n");
        return -EBUSY;
    }

    /* Initialize the graphics context */
    error = gfx_init(&res->gctx);
    if (error != 0) {
        oda_log("oda_init: could not init graphics context\n");
        return error;
    }

    TAILQ_INIT(&res->winq);
    res->cookie = ODA_COOKIE;
    return 0;
}

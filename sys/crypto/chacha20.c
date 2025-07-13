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

#include <crypto/chacha20.h>

static const char sigma[16] = "expand 32-byte k";

void chacha20_init(uint32_t state[16], const uint8_t key[32],
    const uint8_t nonce[12], uint32_t counter)
{
    state[0] = ((uint32_t *)sigma)[0];
    state[1] = ((uint32_t *)sigma)[1];
    state[2] = ((uint32_t *)sigma)[2];
    state[3] = ((uint32_t *)sigma)[3];

    for (int i = 0; i < 8; ++i) {
        state[4 + i] = ((uint32_t *)key)[i];
    }

    state[12] = counter;
    state[13] = ((uint32_t *)nonce)[0];
    state[14] = ((uint32_t *)nonce)[1];
    state[15] = ((uint32_t *)nonce)[2];
}

void
chacha20_block(uint32_t state[16], uint8_t out[64])
{
    uint32_t x[16];
    memcpy(x, state, sizeof(x));

    for (int i = 0; i < 10; i++) {

        QR(x[0], x[4], x[8], x[12]);
        QR(x[1], x[5], x[9], x[13]);
        QR(x[2], x[6], x[10], x[14]);
        QR(x[3], x[7], x[11], x[15]);

        QR(x[0], x[5], x[10], x[15]);
        QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8], x[13]);
        QR(x[3], x[4], x[9], x[14]);
    }

    for (int i = 0; i < 16; ++i) {
        x[i] += state[i];
        ((uint32_t *)out)[i] = x[i];
    }

    state[12]++;
}

void
chacha20_encrypt(uint32_t state[16], uint8_t *in,
    uint8_t *out, size_t len)
{
    uint8_t block[64];
    size_t offset = 0;

    while (len > 0) {
        chacha20_block(state, block);
        size_t n = len > 64 ? 64 : len;

        for (size_t i = 0; i < n; ++i) {
            out[offset + i] = in ? in[offset + i] ^ block[i] : block[i];
        }

        offset += n;
        len -= n;
    }
}

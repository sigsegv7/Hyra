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

#include <sys/sio.h>
#include <sys/device.h>
#include <sys/driver.h>
#include <dev/random/entropy.h>
#include <crypto/chacha20.h>
#include <crypto/siphash.h>
#include <fs/devfs.h>
#include <string.h>

static struct cdevsw random_cdevsw;
static struct entropy_pool entropy;

uint8_t key[32] = {0};
uint8_t nonce[12] = {0};
uint32_t state[16];
uint32_t tsc;

static inline uint64_t
read_tsc(void)
{
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static int
random_read(dev_t dev, struct sio_txn *sio, int flags)
{
    tsc = read_tsc();
    mix_entropy(&entropy, (uint8_t *)&tsc, sizeof(tsc), 1);

    chacha20_init(state, entropy.pool, nonce, 0);
    chacha20_encrypt(state, NULL, sio->buf, sio->len);

    return sio->len;
}

static int
random_init(void)
{
    char devname[] = "random";
    devmajor_t major;
    dev_t dev;

    /* Register the device here */
    major = dev_alloc_major();
    dev = dev_alloc(major);
    dev_register(major, dev, &random_cdevsw);
    devfs_create_entry(devname, major, dev, 0444);

    return 0;
}

static struct cdevsw random_cdevsw = {
    .read = random_read,
    .write = nowrite
};

DRIVER_EXPORT(random_init, "random");

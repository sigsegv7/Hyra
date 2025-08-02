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
#include <sys/dmi.h>
#include <dev/dmi/dmi.h>
#include <dev/dmi/dmivar.h>
#include <fs/ctlfs.h>
#include <string.h>

extern struct ctlops ctl_cpu_ident;

static int
board_ctl_read(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    struct dmi_board board;
    const char *cpu_manuf, *prodver;
    const char *product, *vendor;
    const char *cpu_ver, *p;
    size_t len;

    if (cdp == NULL || sio == NULL) {
        return -EINVAL;
    }
    /* Cannot copy zero bytes */
    if (sio->len == 0) {
        return -EINVAL;
    }

    /* Check offset and clamp length */
    if (sio->offset >= sizeof(board)) {
        return 0;
    }
    if ((sio->offset + sio->len) > sizeof(board)) {
        sio->len = sizeof(board);
    }

    memset(&board, 0, sizeof(board));
    cpu_ver = dmi_cpu_version();
    if (cpu_ver != NULL) {
        len = strlen(cpu_ver);
        memcpy(board.cpu_version, cpu_ver, len);
    }

    prodver = dmi_prodver();
    if (prodver != NULL) {
        len = strlen(prodver);
        memcpy(board.version, prodver, len);
    }

    cpu_manuf = dmi_cpu_manufact();
    if (cpu_manuf != NULL) {
        len = strlen(cpu_manuf);
        memcpy(board.cpu_manuf, cpu_manuf, len);
    }

    product = dmi_product();
    if (product != NULL) {
        len = strlen(product);
        memcpy(board.product, product, len);
    }

    vendor = dmi_vendor();
    if (vendor != NULL) {
        len = strlen(vendor);
        memcpy(board.vendor, vendor, len);
    }

    p = (char *)&board;
    memcpy(sio->buf, &p[sio->offset], sio->len);
    return sio->len;
}

struct ctlops g_ctl_board_ident = {
    .read = board_ctl_read,
    .write = NULL
};

/*
 * Copyright (c) 2023-2024 Ian Marco Moffett and the Osmora Team.
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

#include <fs/initramfs.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/limine.h>
#include <sys/panic.h>
#include <string.h>

__MODULE_NAME("initramfs");
__KERNEL_META("$Hyra$: initramfs.c, Ian Marco Moffett, "
              "Initial ram filesystem");

static volatile struct limine_module_request mod_req = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static const char *initramfs = NULL;
static size_t initramfs_size = 0;

struct tar_hdr {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
};

static char *
get_module(const char *path, uint64_t *size) {
    for (uint64_t i = 0; i < mod_req.response->module_count; ++i) {
        if (strcmp(mod_req.response->modules[i]->path, path) == 0) {
              *size = mod_req.response->modules[i]->size;
              return mod_req.response->modules[i]->address;
        }
    }

    return NULL;
}

static size_t
getsize(const char *in)
{
    size_t size = 0, count = 1;

    for (size_t j = 11; j > 0; --j, count *= 8) {
        size += (in[j-1]-'0')*count;
    }

    return size;
}

static int
initramfs_init(void)
{
    initramfs = get_module("/boot/initramfs.tar", &initramfs_size);

    if (initramfs == NULL) {
        panic("Failed to load initramfs\n");
    }

    return 0;
}

const char *
initramfs_open(const char *path)
{
    uintptr_t addr, size;
    struct tar_hdr *hdr;

    if (initramfs == NULL) {
        initramfs_init();
    }

    if (strlen(path) > 99) {
        return NULL;
    }

    addr = (uintptr_t)initramfs + 0x200;
    hdr = (struct tar_hdr *)addr;

    while (hdr->filename[0] != '\0') {
        hdr = (struct tar_hdr *)addr;
        size = getsize(hdr->size);

        if (strcmp(hdr->filename, path) == 0) {
            return ((char*)addr) + 0x200;
        }

        addr += (((size + 511) / 512) + 1) * 512;
    }

    return NULL;
}

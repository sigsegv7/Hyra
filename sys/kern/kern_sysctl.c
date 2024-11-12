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

#include <sys/sysctl.h>
#include <sys/errno.h>
#include <vm/dynalloc.h>
#include <string.h>

#define HYRA_RELEASE "Hyra/" HYRA_ARCH " " \
        HYRA_VERSION " "              \
        HYRA_BUILDDATE

static const char *hyra = "Hyra";
static const char *hyra_version = HYRA_VERSION;
static const char *osrelease = HYRA_RELEASE;

static struct sysctl_entry common_kerntab[] = {
    [KERN_OSTYPE] = { KERN_OSTYPE, SYSCTL_OPTYPE_STR_RO, &hyra },
    [KERN_OSRELEASE] = { KERN_OSRELEASE, SYSCTL_OPTYPE_STR, &osrelease },
    [KERN_VERSION] = { KERN_VERSION, SYSCTL_OPTYPE_STR, &hyra_version },
};

int
sysctl(struct sysctl_args *args)
{
    struct sysctl_entry *tmp;
    char *tmp_str;
    size_t oldlen, len;

    if (args->name == NULL) {
        return -EINVAL;
    }

    if (args->oldlenp == NULL) {
        return -EINVAL;
    }

    /* TODO: Support writable values */
    if (args->newp != NULL) {
        return -ENOTSUP;
    }

    /*
     * TODO: We only support toplevel names as of now and should
     *       expand this in the future. As of now, users will only
     *       be able to access kern.* entries.
     */
    switch (*args->name) {
    case KERN_OSTYPE:
        tmp = &common_kerntab[KERN_OSTYPE];
        tmp_str = *((char **)tmp->data);
        len = strlen(tmp_str);
        break;
    case KERN_OSRELEASE:
        tmp = &common_kerntab[KERN_OSRELEASE];
        tmp_str = *((char **)tmp->data);
        len = strlen(tmp_str);
        break;
    case KERN_VERSION:
        tmp = &common_kerntab[KERN_VERSION];
        tmp_str = *((char **)tmp->data);
        len = strlen(tmp_str);
        break;
    default:
        return -EINVAL;
    }

    /* Copy new data */
    oldlen = *args->oldlenp;
    memcpy(args->oldp, tmp_str, oldlen);
    return (len > oldlen) ? -ENOMEM : 0;
}

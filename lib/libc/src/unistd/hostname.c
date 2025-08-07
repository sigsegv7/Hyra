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

#include <sys/sysctl.h>
#include <unistd.h>

/*
 * Internal helper to grab a sysctl
 * variable.
 *
 * @name: Name definition of sysctl variable
 * @buf: Buffer to read data in
 * @buflen: Length of buffer
 *
 * Returns zero on success, otherwise a less than
 * zero value.
 */
static int
__sysctl_get(int name, char *buf, size_t buflen)
{
    struct sysctl_args args;
    int error;

    args.name = &name;
    args.nlen = 1;
    args.oldp = buf;
    args.oldlenp = &buflen;
    args.newp = NULL;
    args.newlen = 0;

    if ((error = sysctl(&args)) != 0) {
        return -1;
    }

    return 0;
}

/*
 * Internal helper to set a sysctl
 * variable.
 *
 * @name: Name definition of sysctl variable
 * @buf: Buffer with data to set
 * @buflen: Length of buffer
 *
 * Returns zero on success, otherwise a less than
 * zero value.
 */
static int
__sysctl_set(int name, const char *buf, size_t buflen)
{
    struct sysctl_args args;
    int error;

    args.name = &name;
    args.nlen = 1;
    args.oldp = NULL;
    args.oldlenp = NULL;
    args.newp = (void *)buf;
    args.newlen = buflen;

    if ((error = sysctl(&args)) != 0) {
        return -1;
    }

    return 0;
}

/*
 * Get the system hostname
 *
 * @name: Buffer to read name into
 * @size: Length of name to read
 */
int
gethostname(char *name, size_t size)
{
    if (name == NULL || size == 0) {
        return -1;
    }

    return __sysctl_get(KERN_HOSTNAME, name, size);
}

/*
 * Set the system hostname
 *
 * @name: Name to set
 * @size: Size of name to set
 */
int
sethostname(const char *name, size_t size)
{
    if (name == NULL || size == 0) {
        return -1;
    }

    return __sysctl_set(KERN_HOSTNAME, name, size);
}

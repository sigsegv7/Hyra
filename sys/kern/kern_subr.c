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

#include <sys/proc.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/systm.h>
#include <string.h>

/*
 * Check if a user address is valid.
 *
 * @uaddr: User address to check.
 *
 * Returns true if valid, otherwise false.
 */
static bool
check_uaddr(const void *uaddr)
{
    vaddr_t stack_start, stack_end;
    struct exec_prog exec;
    struct proc *td;
    uintptr_t addr;

    td = this_td();
    exec = td->exec;
    addr = (uintptr_t)uaddr;

    stack_start = td->stack_base;
    stack_end = td->stack_base + PROC_STACK_SIZE;

    if (addr >= exec.start && addr <= exec.end)
        return true;
    if (addr >= stack_start && addr <= stack_end)
        return true;

    return false;
}

/*
 * Copy from userspace to the kernel.
 *
 * @uaddr: Userspace address.
 * @kaddr: Kernelspace address.
 * @len: Length of data.
 */
int
copyin(const void *uaddr, void *kaddr, size_t len)
{
    char *tmp = (char *)uaddr;

    if (!check_uaddr(tmp) || !check_uaddr(tmp + len)) {
        return -EFAULT;
    }

    memcpy(kaddr, uaddr, len);
    return 0;
}

/*
 * Copy from the kernel to userspace.
 *
 * @kaddr: Kernelspace address.
 * @uaddr: Userspace address.
 * @len: Length of data.
 */
int
copyout(const void *kaddr, void *uaddr, size_t len)
{
    char *tmp = uaddr;

    if (!check_uaddr(tmp) || !check_uaddr(tmp + len)) {
        return -EFAULT;
    }

    memcpy(uaddr, kaddr, len);
    return 0;
}

/*
 * Copy in a string from userspace
 *
 * Unlike the typical copyin(), this routine will
 * copy until we've hit NUL ('\0')
 *
 * @uaddr: Userspace address.
 * @kaddr: Kernelspace address.
 * @len: Length of string.
 *
 * XXX: Please note that if `len' is less than the actual
 *      string length, the returned value will not be
 *      NUL terminated.
 */
int
copyinstr(const void *uaddr, char *kaddr, size_t len)
{
    char *dest = (char *)kaddr;
    const char *src = (char *)uaddr;

    if (!check_uaddr(src)) {
        return -EFAULT;
    }

    for (size_t i = 0; i < len; ++i) {
        if (!check_uaddr(src + i)) {
            return -EFAULT;
        }

        dest[i] = src[i];

        if (src[i] == '\0') {
            break;
        }
    }

    return 0;
}

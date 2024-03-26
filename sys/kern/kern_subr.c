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

#include <sys/system.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/sched.h>
#include <vm/vm.h>
#include <string.h>

/*
 * Check if a user address is valid.
 *
 * @uaddr: User address to check.
 *
 * Returns true if valid, otherwise false.
 */
static bool
check_uaddr(uintptr_t uaddr)
{
    struct proc *td = this_td();
    struct vm_range exec_range = td->addr_range[ADDR_RANGE_EXEC];
    struct vm_range stack_range = td->addr_range[ADDR_RANGE_STACK];

    if (uaddr >= exec_range.start && uaddr <= exec_range.end) {
        return true;
    }
    if (uaddr >= stack_range.start && uaddr <= stack_range.end) {
        return true;
    }

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
copyin(uintptr_t uaddr, void *kaddr, size_t len)
{
    if (!check_uaddr(uaddr) || !check_uaddr(uaddr + len)) {
        return -EFAULT;
    }

    memcpy(kaddr, (void *)uaddr, len);
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
copyout(const void *kaddr, uintptr_t uaddr, size_t len)
{
    if (!check_uaddr(uaddr) || !check_uaddr(uaddr + len)) {
        return -EFAULT;
    }

    memcpy((void *)uaddr, kaddr, len);
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
copyinstr(uintptr_t uaddr, char *kaddr, size_t len)
{
    char *dest = (char *)kaddr;
    char *src = (char *)uaddr;

    if (!check_uaddr(uaddr)) {
        return -EFAULT;
    }

    for (size_t i = 0; i < len; ++i) {
        if (!check_uaddr(uaddr + i)) {
            return -EFAULT;
        }

        dest[i] = src[i];

        if (src[i] == '\0') {
            break;
        }
    }

    return 0;
}

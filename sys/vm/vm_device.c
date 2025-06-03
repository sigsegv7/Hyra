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
#include <sys/device.h>
#include <sys/syslog.h>
#include <vm/vm_device.h>

#define pr_trace(fmt, ...) kprintf("vm_device: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

const struct vm_pagerops dv_vnops;

/*
 * Attach a cdev to a vm_object
 *
 * @major: Char device major
 * @minor: Char device minor.
 */
struct vm_object *
dv_attach(devmajor_t major, dev_t dev, vm_prot_t prot)
{
    int error;
    struct cdevsw *cdevp;
    struct vm_object *vmobj;

    if ((cdevp = dev_get(major, dev)) == NULL) {
        pr_error("bad attach (major=%d, dev=%d)\n", major, dev);
        return NULL;
    }

    if (cdevp->mmap == NULL) {
        pr_error("cdev lacks mmap() (major=%d, dev=%d)\n", major, dev);
        return NULL;
    }

    error = vm_obj_init(&cdevp->vmobj, &dv_vnops, 1);
    if (error != 0) {
        return NULL;
    }

    vmobj = &cdevp->vmobj;
    vmobj->prot = prot;
    vmobj->data = cdevp;
    vmobj->pgops = &dv_vnops;
    return vmobj;
}

/* TODO */
const struct vm_pagerops dv_vnops = {
    .get = NULL,
};

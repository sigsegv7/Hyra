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
#include <sys/syscall.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/disk.h>
#include <vm/dynalloc.h>

#define pr_trace(fmt, ...) kprintf("disk: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

/*
 * Clones a disk parameter structure passed
 * by a user. The structure returned is safe
 * to be accessed freely by the kernel.
 *
 * @u_param: Contains user-side pointer
 * @res: Resulting safe data
 *
 * Returns zero on success, otherwise a less than
 * zero value is returned.
 */
static int
disk_param_clone(struct disk_param *u_param, struct disk_param *res)
{
    void *data;
    int error;

    if (u_param == NULL) {
        pr_error("disk_param_clone: got NULL u_param\n");
        return -EINVAL;
    }

    error = copyin(u_param, res, sizeof(*res));
    if (error < 0) {
        return error;
    }

    /*
     * If these parameters do not have a valid cookie, fuck
     * that object, something is not right with it...
     */
    if (res->cookie != DISK_PARAM_COOKIE) {
        pr_error("disk_param_clone: erroneous params (bad cookie)\n");
        return -EACCES;
    }

    data = dynalloc(res->size);
    if (data == NULL) {
        pr_error("disk_param_clone: out of memory\n");
        return -ENOMEM;
    }

    error = copyin(res->buf, data, res->size);
    if (error < 0) {
        pr_error("failed to copy in param data\n");
        dynfree(data);
        return error;
    }

    res->u_buf = res->buf;
    res->buf = data;
    return 0;
}

/*
 * Deallocate a kernel managed disk parameter
 * structure created by disk_param_clone()
 *
 * @param: Params to free
 *
 * Returns zero on success, otherwise a less than
 * zero value is returned.
 */
static int
disk_param_free(struct disk_param *param)
{
    if (param == NULL) {
        return -EINVAL;
    }

    if (param->cookie != DISK_PARAM_COOKIE) {
        return -EACCES;
    }

    dynfree(param->buf);
    return 0;
}

/*
 * Perform an operation on a disk.
 *
 * @id: ID of disk to operate on
 * @opcode: Operation to perform (see DISK_IO_*)
 * @u_param: User side disk parameters
 *
 * Returns a less than zero value on error
 */
static ssize_t
disk_mux_io(diskid_t id, diskop_t opcode, struct disk_param *u_param)
{
    struct disk_param param;
    struct disk *dp;
    ssize_t retval = -EIO;
    int error;

    if (u_param == NULL) {
        return -EINVAL;
    }

    error = disk_param_clone(u_param, &param);
    if (error < 0) {
        return error;
    }

    /* First, attempt to acquire the disk */
    error = disk_get_id(id, &dp);
    if (error < 0) {
        pr_error("disk_mux_io: no such device (id=%d)\n", id);
        return error;
    }

    switch (opcode) {
    case DISK_IO_READ:
        retval = disk_read(
            id,
            param.blk,
            param.buf,
            param.size
        );

        /* Write back the data to the user program */
        error = copyout(param.buf, param.u_buf, param.size);
        if (error < 0) {
            retval = error;
        }
        break;
    case DISK_IO_WRITE:
        retval = disk_write(
            id,
            param.blk,
            param.buf,
            param.size
        );
        break;
    }

    disk_param_free(&param);
    return retval;
}

/*
 * Disk I/O multiplexer syscall
 *
 * arg0: disk id
 * arg1: opcode
 * arg2: disk params
 */
scret_t
sys_disk(struct syscall_args *scargs)
{
    struct disk_param *u_param = (void *)scargs->arg2;
    diskid_t id = scargs->arg0;
    diskop_t opcode = scargs->arg1;

    return disk_mux_io(id, opcode, u_param);
}

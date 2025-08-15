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

#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/disk.h>

/*
 * Disk I/O multiplexer system call which routes
 * various disk operations via a single call.
 *
 * @id: The ID of the disk to be operated on
 * @op: Operation code
 * @param: Operation parameters
 *
 * Returns the number of bytes operated on upon success,
 * otherwise a less than zero value is returned.
 */
ssize_t
__disk_io(diskid_t id, diskop_t op, const struct disk_param *param)
{
    if (param == NULL) {
        return -EINVAL;
    }

    return syscall(
        SYS_disk,
        id,
        op,
        (uintptr_t)param
    );
}

/*
 * Performs a write operation on a specific disk
 *
 * @id: ID of disk to operate on
 * @blk: Block offset to operate on
 * @buf: Data to write
 * @len: Number of bytes to write
 *
 * Returns the number of bytes written upon success, otherwise
 * a less than zero value is returned on error.
 */
ssize_t
disk_write(diskid_t id, blkoff_t blk, const void *buf, size_t len)
{
    struct disk_param param;

    if (buf == NULL || len == 0) {
        return -EINVAL;
    }

    disk_param_init(buf, blk, len, &param);
    return __disk_io(id, DISK_IO_WRITE, &param);
}

/*
 * Performs a read operation on a specific disk
 *
 * @id: ID of disk to operate on
 * @blk: Block offset to operate on
 * @buf: Buffer to read data into
 * @len: Number of bytes to read
 *
 * Returns the number of bytes read upon success, otherwise
 * a less than zero value is returned on error.
 */
ssize_t
disk_read(diskid_t id, blkoff_t blk, void *buf, size_t len)
{
    struct disk_param param;

    if (buf == NULL || len == 0) {
        return -EINVAL;
    }

    disk_param_init(buf, blk, len, &param);
    return __disk_io(id, DISK_IO_READ, &param);
}

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

#ifndef _SYS_DISK_H_
#define _SYS_DISK_H_

#include <sys/syscall.h>
#include <sys/queue.h>
#include <sys/device.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/limits.h>
#include <sys/cdefs.h>
#if defined(_KERNEL)
#include <dev/dcdr/cache.h>
#endif  /* _KERNEL */

#define DISK_NAME_MAX 64

/*
 * V_BSIZE is the virtual block size in bytes used
 * by the disk framework. The virtual block size is a
 * multiple of the hardware block size and defines
 * how many bytes a virtual block is made up of.
 *
 * A virtual block is simply a unit specific to
 * the disk framework that represents multiple
 * hardware disk blocks.
 */
#if defined(__V_BSIZE)
#define V_BSIZE __V_BSIZE
#else
#define V_BSIZE 4096
#endif  /* __V_BSIZE */

/* Sanitize the silly human's input */
_Static_assert(V_BSIZE > 512, "V_BSIZE must be > 512");
_Static_assert((V_BSIZE & 1) == 0, "V_BSIZE must be a power of two");

#define DISK_PRIMARY 0  /* ID of primary disk */

/*
 * To prevent unlikely cases of unintended disk
 * operations (e.g., read, write, etc), we store
 * a cookie within each set of parameters.
 *
 * Requests whose bundle of parameters have no valid
 * cookie shall be rejected by us.
 */
#define DISK_PARAM_COOKIE  0xD1531001

/* Valid disk operations */
#define DISK_IO_READ    0x00
#define DISK_IO_WRITE   0x01

/*
 * A disk identifier is a zero-based index into
 * the disk registry.
 */
typedef uint16_t diskid_t;

/*
 * Block offset / LBA
 */
typedef off_t blkoff_t;

/*
 * Disk operations may be requested by user
 * programs by using a disk operation code.
 */
typedef uint8_t diskop_t;

/*
 * The disk metadata structure contains information
 * describing the disk. It is used for Hyra's pbuf
 * (persistent buffers / sls) support. This structure
 * is to be stored at the very last sector of the drive.
 *
 * @root_blk: Disk offset to root block
 * @n_ublk: Number of usable user blocks
 */
struct disk_meta {
    char magic[6];
    blkoff_t root_blk;
    size_t n_ublk;
};

/*
 * A disk I/O parameter contains information
 * that is passed from a user application to
 * the kernel for specific operations.
 *
 * @buf: User-side pointer to data buffer
 * @size: Size of data buffer in bytes
 * @cookie: Used to prevent unintended operations
 * @blk: Disk block offset
 * @u_buf: Used by the kernel to keep track of user buffer
 */
struct disk_param {
    void *buf;
    size_t size;
    uint32_t cookie;
    blkoff_t blk;
#if defined(_KERNEL)
    void *u_buf;
#endif
};

/*
 * Helper used to initialize disk I/O parameters.
 * This is used by the user to initialize a declared
 * set of parameters.
 *
 * @buf: Buffer to operate on
 * @blk: Disk block to operate on
 * @size: Operation size in bytes (block-aligned)
 * @res: Pointer to params to be initialized
 */
__always_inline static inline void
disk_param_init(uint8_t *buf, blkoff_t blk, size_t size, struct disk_param *res)
{
    if (res != NULL) {
        res->buf = buf;
        res->blk = blk;
        res->size = size;
        res->cookie = DISK_PARAM_COOKIE;
    }
}

/*
 * User side disk API
 */
#if !defined(_KERNEL)
ssize_t __disk_io(diskid_t id, diskop_t op, const struct disk_param *param);
ssize_t disk_write(diskid_t id, blkoff_t off, const void *buf, size_t len);
ssize_t disk_read(diskid_t id, blkoff_t off, void *buf, size_t len);
#endif  /* !_KERNEL */

#if defined(_KERNEL)
/*
 * Represents a block storage device
 *
 * @name: Name of disk
 * @cookie: Used internally to ensure validity
 * @bsize: Hardware block size (defaults to 512 bytes)
 * @dev: Device minor
 * @id: Disk ID (zero-based index)
 * @bdev: Block device operations
 * @link: TAILQ link
 */
struct disk {
    char name[DISK_NAME_MAX];
    uint32_t cookie;
    uint16_t bsize;
    dev_t dev;
    diskid_t id;
    const struct bdevsw *bdev;
    TAILQ_ENTRY(disk) link;
};

void *disk_buf_alloc(diskid_t id, size_t len);
void disk_buf_free(void *p);

ssize_t disk_read(diskid_t id, blkoff_t blk, void *buf, size_t len);
ssize_t disk_write(diskid_t id, blkoff_t blk, const void *buf, size_t len);

int disk_add(const char *name, dev_t dev, const struct bdevsw *bdev, int flags);
int disk_get_id(diskid_t id, struct disk **res);

scret_t sys_disk(struct syscall_args *scargs);
#endif  /* _KERNEL */
#endif  /* !_SYS_DISK_H_ */

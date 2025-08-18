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
#include <sys/queue.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/sio.h>
#include <sys/param.h>
#include <sys/panic.h>
#include <sys/spinlock.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <vm/dynalloc.h>
#include <assert.h>
#include <string.h>

#define pr_trace(fmt, ...) kprintf("disk: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define DEFAULT_BSIZE 512       /* Default block size in bytes */
#define DISKQ_COOKIE 0xD9EA     /* Verification cookie */

/*
 * The maximum disks supported by the kernel
 * is defined by the `DISK_MAX' kconf(9) option.
 *
 * We define a default of 16 if that option is not
 * specified.
 */
#if defined(__DISK_MAX)
#define DISK_MAX __DISK_MAX
#else
#define DISK_MAX 16             /* Maximum disks */
#endif

/*
 * We set a hard limit at 64 disks to prevent misconfiguration as
 * it is unlikely that one would ever have that many on a single
 * instance. Though of course, anything is possible, so one may
 * patch the hard limit defined below to a higher value if needed.
 */
__static_assert(DISK_MAX < 64, "DISK_MAX exceeds hard limit");

/*
 * The disk queue stores descriptors of disks that
 * are registered with the system. This allows for
 * easy and simplified access of the storage medium.
 *
 * XXX: An array would be more efficent, however disks
 *      could be detached or swapped during runtime thus
 *      making the usage of queues a more sane design.
 *
 *      This also provides the added benefit of lazy-allocation
 *      so memory isn't wasted and only allocated when we actually
 *      have a disk descriptor that it would be used to store.
 */
static struct spinlock diskq_lock;
static TAILQ_HEAD(, disk) diskq;
static uint16_t disk_count = 0;
static uint16_t diskq_cookie = 0;

/*
 * Verify that a disk descriptor has been properly
 * initialized by comparing against the cookie field.
 *
 * Returns a value of zero if valid, otherwise a less
 * than zero value is returned.
 */
__always_inline static inline int
check_disk_cookie(struct disk *dp)
{
    __assert(dp != NULL);
    return (dp->cookie == DISKQ_COOKIE) ? 0 : -1;
}

/*
 * Verify if the disk queue is initialized and
 * ready for descriptors to be added.
 *
 * Returns a value of zero if it has already been
 * initialized, otherwise a value less than zero
 * is returned after check_diskq() initializes
 * the disk queue.
 */
static inline int
check_diskq(void)
{
    if (diskq_cookie != DISKQ_COOKIE) {
        TAILQ_INIT(&diskq);
        diskq_cookie = DISKQ_COOKIE;
        return -1;
    }

    return 0;
}

/*
 * Acquire a disk descriptor through a zero-based
 * disk index. Returns a pointer to the disk descriptor
 * on success, otherwise a less than zero value is returned.
 *
 * @id: Disk index
 *
 * XXX: This is the lockless internal implementation,
 *      please use disk_get_id() instead.
 */
static struct disk *
__disk_get_id(diskid_t id)
{
    struct disk *dp;

    if (id >= disk_count) {
        return NULL;
    }

    dp = TAILQ_FIRST(&diskq);
    if (dp == NULL) {
        return NULL;
    }

    /*
     * Now, we start at the first disk entry and
     * traverse the list. If the ID of a disk matches
     * the ID we are looking for, return it.
     */
    while (dp != NULL) {
        if (dp->id == id) {
            return dp;
        }

        dp = TAILQ_NEXT(dp, link);
    }

    /* Nothing found :( */
    return NULL;
}

/*
 * Attempt to perform a read/write operation on
 * a disk.
 *
 * @id: ID of disk to operate on
 * @blk: Block offset to read at
 * @buf: Buffer to read data into
 * @len: Number of bytes to read
 * @write: If true, do a write
 *
 * XXX: The size in which blocks are read at is in
 *      virtual blocks which is defined by V_BSIZE
 *      in sys/disk.h
 */
static ssize_t
disk_rw(diskid_t id, blkoff_t blk, void *buf, size_t len, bool write)
{
    const struct bdevsw *bdev;
    struct sio_txn sio;
    struct disk *dp;
    int error;

    len = ALIGN_UP(len, V_BSIZE);

    /* Attempt to grab the disk object */
    error = disk_get_id(id, &dp);
    if (error < 0) {
        return error;
    }

    /* Sanity check, should not happen */
    bdev = dp->bdev;
    if (__unlikely(bdev == NULL)) {
        return -EIO;
    }

    /* Prepare the buffer */
    sio.buf = buf;
    sio.offset = blk * dp->bsize;
    sio.len = len;

    /* Handle writes */
    if (write) {
        if (bdev->write == NULL) {
            return -ENOTSUP;
        }

        return bdev->write(dp->dev, &sio, 0);
    }

    /* Do we support this operation? */
    if (bdev->read == NULL) {
        return -ENOTSUP;
    }

    return bdev->read(dp->dev, &sio, 0);
}

/*
 * Register a disk with the system so that it may
 * be accessible independently of its device major
 * and minor numbers
 *
 * @name: Name of the disk
 * @dev: Device minor
 * @bdev: Block device operations associated with device
 *
 * Returns zero on success, otherwise a less than zero
 * value is returned.
 */
int
disk_add(const char *name, dev_t dev, const struct bdevsw *bdev, int flags)
{
    struct disk *dp;
    size_t name_len;

    if (name == NULL || bdev == NULL) {
        return -EINVAL;
    }

    /* Disk queue must be initialized */
    check_diskq();

    /* There is a limit to how many can be added */
    if (disk_count >= DISK_MAX) {
        pr_error("disk_add: disk limit %d/%d reached\n",
            disk_count, DISK_MAX);
        return -EAGAIN;
    }

    /* Is the disk name of correct length? */
    name_len = strlen(name);
    if (name_len >= sizeof(dp->name) - 1) {
        pr_error("disk_add: name too big (len=%d)\n", name_len);
        return -E2BIG;
    }

    dp = dynalloc(sizeof(*dp));
    if (dp == NULL) {
        pr_error("failed to allocate disk\n");
        return -ENOMEM;
    }

    /* Initialize the descriptor */
    memset(dp, 0, sizeof(*dp));
    memcpy(dp->name, name, name_len);
    dp->cookie = DISKQ_COOKIE;
    dp->bdev = bdev;
    dp->dev = dev;
    dp->id = disk_count++;
    dp->bsize = DEFAULT_BSIZE;

    /*
     * We are to panic if the virtual blocksize
     * defined is not a multiple of any hardware
     * block size
     */
    if ((V_BSIZE & (dp->bsize - 1)) != 0) {
        panic("virtual block size not hw bsize aligned\n");
    }

    /* Now we can add it to the queue */
    spinlock_acquire(&diskq_lock);
    TAILQ_INSERT_TAIL(&diskq, dp, link);
    spinlock_release(&diskq_lock);
    return 0;
}

/*
 * Acquire a disk descriptor by using a zero-based
 * index.
 *
 * @id: Disk index (0: primary)
 * @res: Resulting disk descriptor
 *
 * Returns zero on success, otherwise a less than
 * zero value is returned.
 */
int
disk_get_id(diskid_t id, struct disk **res)
{
    int error;
    struct disk *dp;

    if (res == NULL) {
        return -EINVAL;
    }

    if (id >= disk_count) {
        return -ENODEV;
    }

    /* Grab the disk */
    spinlock_acquire(&diskq_lock);
    dp = __disk_get_id(id);
    spinlock_release(&diskq_lock);

    /* Did it even exist? */
    if (dp == NULL) {
        return -ENODEV;
    }

    /* Should not fail but make sure */
    error = check_disk_cookie(dp);
    if (__unlikely(error < 0)) {
        panic("disk_get_id: got bad disk object\n");
    }

    *res = dp;
    return 0;
}

/*
 * Allocate a memory buffer that may be used for
 * disk I/O.
 *
 * @id: ID of disk buffer will be used for
 * @len: Length to allocate
 */
void *
disk_buf_alloc(diskid_t id, size_t len)
{
    struct disk *dp;
    void *buf;

    if (len == 0) {
        return NULL;
    }

    /* Attempt to acquire the disk */
    if (disk_get_id(id, &dp) < 0) {
        return NULL;
    }

    /*
     * Here we will align the buffer size by the
     * virtual block size to ensure it is big enough.
     */
    len = ALIGN_UP(len, V_BSIZE);
    buf = dynalloc(len);
    return buf;
}

/*
 * Free a memory buffer that was allocated by
 * disk_buf_alloc()
 */
void
disk_buf_free(void *p)
{
    if (p != NULL) {
        dynfree(p);
    }
}

/*
 * Attempt to perform a read operation on
 * a disk.
 *
 * @id: ID of disk to operate on
 * @blk: Block offset to read at
 * @buf: Buffer to read data into
 * @len: Number of bytes to read
 */
ssize_t
disk_read(diskid_t id, blkoff_t blk, void *buf, size_t len)
{
    ssize_t retval;
    char *tmp;

    tmp = disk_buf_alloc(id, len);
    if (tmp == NULL) {
        return -ENOMEM;
    }

    retval = disk_rw(id, blk, tmp, len, false);
    if (retval < 0) {
        disk_buf_free(tmp);
        return retval;
    }

    memcpy(buf, tmp, len);
    disk_buf_free(tmp);
    return retval;
}

/*
 * Attempt to perform a write operation on
 * a disk.
 *
 * @id: ID of disk to operate on
 * @blk: Block offset to read at
 * @buf: Buffer containing data to write
 * @len: Number of bytes to read
 */
ssize_t
disk_write(diskid_t id, blkoff_t blk, const void *buf, size_t len)
{
    ssize_t retval;
    char *tmp;

    tmp = disk_buf_alloc(id, len);
    if (tmp == NULL) {
        return -ENOMEM;
    }

    memcpy(tmp, buf, len);
    retval  = disk_rw(id, blk, tmp, len, true);
    disk_buf_free(tmp);
    return retval;
}

/*
 * Attempt to request attributes from a specific
 * device.
 *
 * @id: ID of disk to query
 * @res: Resulting information goes here
 *
 * This function returns zero on success, otherwise
 * a less than zero value is returned.
 */
int
disk_query(diskid_t id, struct disk_info *res)
{
    const struct bdevsw *bdev;
    struct disk *dp;
    int error;

    if (res == NULL) {
        return -EINVAL;
    }

    /* Attempt to grab the disk */
    error = disk_get_id(id, &dp);
    if (error < 0) {
        pr_error("disk_query: bad disk ID %d\n", id);
        return error;
    }

    bdev = dp->bdev;
    if (__unlikely(bdev == NULL)) {
        pr_error("disk_query: no bdev for disk %d\n", id);
        return -EIO;
    }

    res->block_size = dp->bsize;
    res->vblock_size = V_BSIZE;
    res->n_block = bdev->bsize(dp->dev);
    return 0;
}

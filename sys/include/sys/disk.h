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

#include <sys/queue.h>
#include <sys/device.h>
#include <sys/types.h>
#include <sys/limits.h>

#define DISK_PRIMARY 0  /* ID of primary disk */

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
 * Represents a block storage device
 *
 * @name: Name of disk
 * @cookie: Used internally to ensure validity
 * @bsize: Block size (defaults to 512 bytes)
 * @dev: Device minor
 * @id: Disk ID (zero-based index)
 * @bdev: Block device operations
 * @link: TAILQ link
 */
struct disk {
    char name[NAME_MAX];
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

#endif  /* !_SYS_DISK_H_ */

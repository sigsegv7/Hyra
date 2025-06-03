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

#ifndef _SYS_DEVICE_H_
#define _SYS_DEVICE_H_

#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/sio.h>
#include <vm/vm_obj.h>

typedef uint8_t devmajor_t;

/* Device operation typedefs */
typedef int(*dev_read_t)(dev_t, struct sio_txn *, int);
typedef int(*dev_write_t)(dev_t, struct sio_txn *, int);
typedef int(*dev_bsize_t)(dev_t);

struct cdevsw {
    int(*read)(dev_t dev, struct sio_txn *sio, int flags);
    int(*write)(dev_t dev, struct sio_txn *sio, int flags);
    paddr_t(*mmap)(dev_t dev, off_t off, int flags);

    /* Private */
    struct vm_object vmobj;
};

struct bdevsw {
    int(*read)(dev_t dev, struct sio_txn *sio, int flags);
    int(*write)(dev_t dev, struct sio_txn *sio, int flags);
    int(*bsize)(dev_t dev);
};

void *dev_get(devmajor_t major, dev_t dev);
dev_t dev_alloc(devmajor_t major);

devmajor_t dev_alloc_major(void);
int dev_register(devmajor_t major, dev_t dev, void *devsw);

int dev_noread(void);
int dev_nowrite(void);
int dev_nobsize(void);

/* Device operation stubs */
#define noread ((dev_read_t)dev_noread)
#define nowrite ((dev_write_t)dev_nowrite)
#define nobsize ((dev_bsize_t)dev_nobsize)

#endif  /* _KERNEL */
#endif  /* !_SYS_DEVICE_H_ */

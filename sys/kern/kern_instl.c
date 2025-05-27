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

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/syslog.h>
#include <sys/sio.h>
#include <sys/vnode.h>
#include <sys/disklabel.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/sched.h>
#include <sys/reboot.h>
#include <machine/cdefs.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <dev/timer.h>
#include <assert.h>
#include <string.h>

#define DEFAULT_TIMEOUT 3
#define YIELD_TIMEOUT 200000  /* In usec */
#define BLOCK_SIZE 512
#define BLOCK_THRESHOLD (BLOCK_SIZE * 1024)

struct progress_bar {
    bool dec;
    uint8_t progress;
};

__used static struct timer tmr;
__used static struct timer sched_timer;
__used static struct sio_txn hdd_sio;

#if defined(_INSTALL_MEDIA)
__dead static void
installer_quit(uint32_t seconds)
{
    kprintf("restarting in %d seconds...\n", seconds);
    tmr.msleep(seconds * 1000);
    cpu_reboot(REBOOT_RESET);
    __builtin_unreachable();
}

static inline void
installer_yield(void)
{
    md_inton();
    sched_timer.oneshot_us(YIELD_TIMEOUT);
    md_hlt();
    md_intoff();
}

/*
 * Throttle CPU usage by giving it small
 * breaks based on the amount of data already
 * read. The installer needs to perform very
 * large block I/O operations and we want to
 * avoid significant temperature spikes that
 * would be kind of scary :(
 *
 * @n: Number of bytes read
 */
static void
installer_throttle(size_t n)
{
    if ((n % BLOCK_THRESHOLD) == 0) {
        installer_yield();
    }
}

/*
 * Create a progress bar animation for long
 * operations.
 *
 * @bp: Pointer to a progress bar structure.
 * @n: Number of blocks operated on.
 * @max: Max blocks per bar update.
 */
static void
progress_update(struct progress_bar *bp, size_t n, size_t max)
{
    const char CHR = '.';

    /*
     * We only want to update the progress bar
     * per `max' blocks written.
     */
    if ((n > 0) && (n % max) != 0) {
        return;
    }

    /* Add more '.' chars */
    if (bp->progress < 8 && !bp->dec) {
        kprintf(OMIT_TIMESTAMP "%c\f", CHR);
    } else if (bp->progress >= 8) {
        bp->dec = true;
    }

    /* Remove '.' chars */
    if (bp->dec && bp->progress > 0) {
        kprintf(OMIT_TIMESTAMP "\b\f");
    } else if (bp->progress == 0) {
        bp->dec = false;
    }

    if (!bp->dec) {
        ++bp->progress;
    } else {
        --bp->progress;
    }
}

static void
installer_wipe(struct filedesc *hdd, uint32_t count)
{
    struct sio_txn sio;
    struct progress_bar bar = {0, 0};
    size_t write_len, total_blocks;
    size_t write_blocks;
    char buf[BLOCK_SIZE * 2];

    write_len = sizeof(buf);
    memset(buf, 0, write_len);
    write_blocks = write_len / BLOCK_SIZE;

    total_blocks = ALIGN_UP(count, BLOCK_SIZE);
    total_blocks /= BLOCK_SIZE;

    if (__unlikely(total_blocks == 0)) {
        kprintf("bad block size for /dev/sd1\n");
        installer_quit(DEFAULT_TIMEOUT);
    }

    sio.buf = buf;
    sio.offset = 0;
    sio.len = write_len;

    /* Zero that shit */
    kprintf("zeroing %d blocks...\n", total_blocks);
    for (int i = 0; i < total_blocks; i += write_blocks) {
        vfs_vop_write(hdd->vp, &sio);
        installer_throttle(sio.offset);
        sio.offset += write_len;
        progress_update(&bar, i, 1000);
    }

    /* Cool off then continue */
    installer_yield();
    hdd_sio.offset = 0;
    kprintf(OMIT_TIMESTAMP "OK\n");
    tmr.msleep(1000);
}

/*
 * Write data to the drive.
 *
 * @hdd: HDD file descriptor
 * @file: Optional source file descriptor
 * @p: Data pointer
 * @len: Length of data.
 */
static void
installer_write(struct filedesc *hdd, struct filedesc *file, void *p, size_t len)
{
    size_t nblocks;
    struct sio_txn sio;
    struct progress_bar bar = {0, 0};
    char buf[BLOCK_SIZE];

    len = ALIGN_UP(len, BLOCK_SIZE);
    nblocks = len / BLOCK_SIZE;

    hdd_sio.len = BLOCK_SIZE;
    hdd_sio.buf = (len < BLOCK_SIZE) ? buf : p;

    sio.len = BLOCK_SIZE;
    sio.offset = 0;
    sio.buf = (len < BLOCK_SIZE) ? buf : p;

    if (len < BLOCK_SIZE) {
        memcpy(buf, p, len);
    }

    kprintf("writing %d block(s)...\n", nblocks);
    for (size_t i = 0; i < nblocks; ++i) {
        if (file != NULL) {
            vfs_vop_read(file->vp, &sio);
        }
        vfs_vop_write(hdd->vp, &hdd_sio);
        installer_throttle(hdd_sio.offset);

        sio.offset += BLOCK_SIZE;
        hdd_sio.offset += BLOCK_SIZE;
        progress_update(&bar, i, 400);
    }

    kprintf(OMIT_TIMESTAMP "OK\n");
}

#endif

int
hyra_install(void)
{
#if defined(_INSTALL_MEDIA)
    int fd, hdd_fd;
    char buf[BLOCK_SIZE];
    struct filedesc *fildes, *hdd_fildes;
    struct disklabel label;
    struct vop_getattr_args getattr_args;
    struct vattr iso_attr;
    size_t iso_size;
    size_t nzeros = 0;  /* Zero byte count */
    tmrr_status_t tmr_error;

    /* Needed for msleep() */
    tmr_error = req_timer(TIMER_GP, &tmr);
    if (__unlikely(tmr_error != TMRR_SUCCESS)) {
        kprintf("could not fetch TIMER_GP\n");
        installer_quit(DEFAULT_TIMEOUT);
    }

    /*
     * Grab the scheduler timer since we can
     * reasonably assume it has oneshot capability.
     */
    tmr_error = req_timer(TIMER_SCHED, &sched_timer);
    if (__unlikely(tmr_error != TMRR_SUCCESS)) {
        kprintf("could not fetch TIMER_SCHED\n");
        installer_quit(DEFAULT_TIMEOUT);
    }

    kprintf("::::::::::::::::::::::::::::\n");
    kprintf("::::: Hyra Installer  ::::::\n");
    kprintf("::::::::::::::::::::::::::::\n");
    kprintf("!! DRIVE WILL BE WIPED !!\n");
    tmr.msleep(5000);


    /*
     * See if the target drive exists
     *
     * XXX: As of now, we only support SATA drives
     *      as a target for the installer.
     */
    hdd_fd = fd_open("/dev/sd1", O_RDWR);
    if (hdd_fd < 0) {
        kprintf("could not open /dev/sd1\n");
        installer_quit(DEFAULT_TIMEOUT);
    }

    kprintf("installing to /dev/sd1...\n");

    fd = fd_open("/boot/Hyra.iso", O_RDONLY);
    if (fd < 0) {
        kprintf("could not open /boot/Hyra.iso (status=%d)\n", fd);
        installer_quit(DEFAULT_TIMEOUT);
    }

    /* Get attributes */
    fildes = fd_get(fd);
    getattr_args.vp = fildes->vp;
    getattr_args.res = &iso_attr;
    vfs_vop_getattr(fildes->vp, &getattr_args);

    /* Get the ISO size */
    iso_size = ALIGN_UP(iso_attr.size, BLOCK_SIZE);

    /*
     * First, wipe part of the drive of any data.
     * This is done by simply filling it with
     * zeros.
     */
    hdd_fildes = fd_get(hdd_fd);
    nzeros = iso_size + sizeof(struct disklabel);
    nzeros += BLOCK_SIZE;
    installer_wipe(hdd_fildes, nzeros);

    /*
     * Now since the drive is zerored, we can prep
     * our data buffers to write the actual install.
     */
    label.magic = DISK_MAG;
    label.sect_size = BLOCK_SIZE;
    installer_write(hdd_fildes, fildes, buf, iso_size);
    installer_write(hdd_fildes, NULL, &label, sizeof(label));

    kprintf("Installation complete!\n");
    kprintf("Please remove installation media\n");
    installer_quit(5);
#endif  /* _INSTALL_MEDIA */
    return 0;
}

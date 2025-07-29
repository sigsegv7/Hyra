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

#include <sys/limits.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/filedesc.h>

/*
 * Read data into POSIX.1‐2017 iovec
 *
 * @filedes: File descriptor number
 * @iov: I/O vector to read file into
 * @iovnt: Number of I/O vectors
 */
ssize_t
readv(int filedes, const struct iovec *iov, int iovcnt)
{
    void *base;
    size_t len;
    ssize_t tmp, bytes_read = 0;

    if (filedes < 0) {
        return -EINVAL;
    }

    /*
     * Make sure that this conforms to our max
     * iovec limit.
     */
    if (iovcnt > IOVEC_MAX) {
        return -EINVAL;
    }

    /*
     * Go through each I/O vector and read a chunk
     * of data into one.
     */
    for (int i = 0; i < iovcnt; ++i) {
        base = iov[i].iov_base;
        len = iov[i].iov_len;

        /*
         * If we encounter a base that is NULL,
         * or if the length to read is an invalid
         * value of zero. We can just assume this
         * is some sort of weird list termination?
         */
        if (base == NULL || len == 0) {
            break;
        }

        /* Read the file into this base  */
        tmp = fd_read(filedes, base, len);

        /* Did anything go wrong? */
        if (tmp < 0) {
            return tmp;
        }

        /* No more data */
        if (tmp == 0) {
            break;
        }

        /* Read more bytes */
        bytes_read += tmp;
    }

    return bytes_read;
}

/*
 * Write data from POSIX.1‐2017 iovec
 *
 * @filedes: File descriptor number
 * @iov: I/O vector to write to file
 * @iovnt: Number of I/O vectors
 */
ssize_t
writev(int filedes, const struct iovec *iov, int iovcnt)
{
    void *base;
    size_t len;
    ssize_t bytes_written = 0;
    ssize_t tmp;

    if (filedes < 0) {
        return -EINVAL;
    }

    /*
     * Are we within the limits? Return an
     * error if not.
     */
    if (iovcnt > IOVEC_MAX) {
        return -EINVAL;
    }

    for (int i = 0; i < iovcnt; ++i) {
        base = iov[i].iov_base;
        len = iov[i].iov_len;

        /*
         * These are invalid, whatever these are,
         * terminate our walk through.
         */
        if (base == NULL || len == 0) {
            break;
        }

        /* Write the data from the iovec */
        tmp = fd_write(filedes, base, len);

        /* Was there an error? */
        if (tmp < 0) {
            return tmp;
        }

        /* No more data to read? */
        if (tmp == 0) {
            break;
        }

        bytes_written += tmp;
    }

    return bytes_written;
}

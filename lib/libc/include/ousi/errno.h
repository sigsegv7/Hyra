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

#ifndef _OUSI_ERRNO_H_
#define _OUSI_ERRNO_H_

#define EPERM           1       /* Not super-user */
#define ENOENT          2       /* No such file or directory */
#define ESRCH           3       /* No such process */
#define EINTR           4       /* Interrupted system call */
#define EIO             5       /* I/O error */
#define ENXIO           6       /* No such device or address */
#define E2BIG           7       /* Arg list too long */
#define ENOEXEC         8       /* Exec format error */
#define EBADF           9       /* Bad file number */
#define ECHILD         10       /* No children */
#define EAGAIN         11       /* No more processes */
#define ENOMEM         12       /* Not enough core */
#define EACCES         13       /* Permission denied */
#define EFAULT         14       /* Bad address */
#define ENOTBLK        15       /* Block device required */
#define EBUSY          16       /* Mount device busy */
#define EEXIST         17       /* File exists */
#define EXDEV          18       /* Cross-device link */
#define ENODEV         19       /* No such device */
#define ENOTDIR        20       /* Not a directory */
#define EISDIR         21       /* Is a directory */
#define EINVAL         22       /* Invalid argument */
#define ENFILE         23       /* Too many open files in system */
#define EMFILE         24       /* Too many open files */
#define ENOTTY         25       /* Not a typewriter */
#define ETXTBSY        26       /* Text file busy */
#define EFBIG          27       /* File too large */
#define ENOSPC         28       /* No space left on device */
#define ESPIPE         29       /* Illegal seek */
#define EROFS          30       /* Read only file system */
#define EMLINK         31       /* Too many links */
#define EPIPE          32       /* Broken pipe */
#define ETIME          62       /* Timer expired */
#define ENOLINK        67       /* The link has been severed */
#define EPROTO         71       /* Protocol error */
#define EMULTIHOP      74       /* Multihop attempted */
#define EBADMSG        77       /* Trying to read unreadable message */
#define EBADFD         81       /* f.d. invalid for this operation */
#define ENOTEMPTY      90       /* Directory not empty */
#define ENAMETOOLONG   91       /* File or path name too long */
#define ELOOP          92       /* Too many symbolic links */
#define EOPNOTSUPP     95       /* Operation not supported on transport endpoint */
#define EPFNOSUPPORT   96       /* Protocol family not supported */
#define ECONNRESET     104      /* Connection reset by peer */
#define ENOBUFS        105      /* No buffer space available */
#define EAFNOSUPPORT   106      /* Address family not supported by protocol family */
#define EPROTOTYPE     107      /* Protocol wrong type for socket */
#define ENOTSOCK       108      /* Socket operation on non-socket */
#define ENOPROTOOPT    109      /* Protocol not available */
#define ESHUTDOWN      110      /* Can't send after socket shutdown */
#define ECONNREFUSED   111      /* Connection refused */
#define EADDRINUSE     112      /* Address already in use */
#define ECONNABORTED   113      /* Connection aborted */
#define ENETUNREACH    114      /* Network is unreachable */
#define ENETDOWN       115      /* Network interface is not configured */
#define ETIMEDOUT      116      /* Connection timed out */
#define EHOSTDOWN      117      /* Host is down */
#define EHOSTUNREACH   118      /* Host is unreachable */
#define EINPROGRESS    119      /* Connection already in progress */
#define EALREADY       120      /* Socket already connected */
#define EDESTADDRREQ   121      /* Destination address required */
#define EMSGSIZE       122      /* Message too long */
#define EPROTONOSUPPORT 123     /* Unknown protocol */
#define ESOCKTNOSUPPORT 124     /* Socket type not supported */
#define EADDRNOTAVAIL  125      /* Address not available */
#define EISCONN        127      /* Socket is already connected */
#define ENOTCONN       128      /* Socket is not connected */
#define ENOTSUP        134      /* Not supported */
#define EOVERFLOW      139      /* Value too large for defined data type */

#endif  /* !_OUSI_ERRNO_H_ */

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

#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <sys/socketvar.h>
#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/mutex.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif  /* _KERNEL */

#ifndef _SA_FAMILY_T_DEFINED_
#define _SA_FAMILY_T_DEFINED_
typedef uint32_t sa_family_t;
#endif  /* _SA_FAMILY_T_DEFINED_ */

#ifndef _SOCKLEN_T_DEFINED_
#define _SOCKLEN_T_DEFINED_
typedef uint32_t socklen_t;
#endif  /* !_SOCKLEN_T_DEFINED_ */

/*
 * Address family defines
 */
#define AF_UNSPEC 0
#define AF_UNIX   1
#define AF_LOCAL  AF_UNIX

/* Socket types */
#define SOCK_STREAM 1

struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[108];
};

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};

#if defined(_KERNEL)

struct ksocket {
    int sockfd;
    union {
        struct sockaddr sockaddr;
        struct sockaddr_un un;
    };
    struct sockbuf buf;
    struct mutex *mtx;
};

scret_t sys_socket(struct syscall_args *scargs);
scret_t sys_bind(struct syscall_args *scargs);
scret_t sys_recv(struct syscall_args *scargs);
scret_t sys_send(struct syscall_args *scargs);
#endif  /* _KERNEL */

int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t len);

ssize_t send(int sockfd, const void *buf, size_t size, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);

#endif  /* !_SYS_SOCKET_H_ */

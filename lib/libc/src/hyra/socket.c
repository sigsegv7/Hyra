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

#include <sys/socket.h>
#include <sys/syscall.h>

int
socket(int domain, int type, int protocol)
{
    return syscall(SYS_socket, domain, type, protocol);
}

int
bind(int sockfd, const struct sockaddr *addr, socklen_t len)
{
    return syscall(SYS_bind, sockfd, (uintptr_t)addr, len);
}

ssize_t
send(int sockfd, const void *buf, size_t size, int flags)
{
    return syscall(SYS_send, sockfd, (uintptr_t)buf, size, flags);
}

ssize_t
recv(int sockfd, void *buf, size_t len, int flags)
{
    return syscall(SYS_recv, sockfd, (uintptr_t)buf, len, flags);
}

ssize_t
sendmsg(int socket, const struct msghdr *msg, int flags)
{
    return syscall(SYS_sendmsg, socket, (uintptr_t)msg, flags);
}

ssize_t
recvmsg(int socket, struct msghdr *msg, int flags)
{
    return syscall(SYS_recvmsg, socket, (uintptr_t)msg, flags);
}

int
connect(int socket, const struct sockaddr *address, socklen_t len)
{
    return syscall(SYS_connect, socket, (uintptr_t)address, len);
}

int
setsockopt(int sockfd, int level, int name, const void *v, socklen_t len)
{
    return syscall(
        SYS_setsockopt,
        sockfd,
        level,
        name,
        (uintptr_t)v,
        len
    );
}

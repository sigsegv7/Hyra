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

#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/exec.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <stddef.h>

#define F_OK 0

/* lseek whence, follows Hyra ABI */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

__BEGIN_DECLS

int sysconf(int name);
int setuid(uid_t new);

int gethostname(char *name, size_t size);
int sethostname(const char *name, size_t size);

uid_t getuid(void);
char *getlogin(void);

char *getcwd(char *buf, size_t size);
char *getwd(char *pathname);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

int close(int fd);
int access(const char *path, int mode);

off_t lseek(int fildes, off_t offset, int whence);
int unlinkat(int dirfd, const char *pathname, int flags);
int unlink(const char *path);

int dup(int fd);
int dup2(int fd, int fd1);

pid_t getpid(void);
pid_t getppid(void);
pid_t fork(void);

extern char *optarg;
extern int optind, opterr, optopt;

int getopt(int argc, char *argv[], const char *optstring);

__END_DECLS

#endif  /* !_UNISTD_H */

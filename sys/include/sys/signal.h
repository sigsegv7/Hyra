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

#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/proc.h>

#define SIGFPE      8   /* Floating point exception */
#define SIGKILL     9   /* Kill */
#define SIGSEGV     11  /* Segmentation violation */
#define SIGTERM     15  /* Terminate gracefully */

typedef uint32_t sigset_t;

typedef struct {
    int si_signo;
    int si_code;
} siginfo_t;

struct sigaction {
    void(*sa_handler)(int signo);
    sigset_t sa_mask;
    int sa_flags;
    void(*sa_sigaction)(int signo, siginfo_t *si, void *p);
};

#if defined(_KERNEL)
struct proc;

struct ksiginfo {
    int signo;
    int sigcode;
    struct sigaction *si;
    TAILQ_ENTRY(ksiginfo) link;
};

/* Signal management */
int newsig(struct proc *td, int signo, struct ksiginfo **ksig);
int delsig(struct proc *td, int signo);
int sendsig(struct proc *td, const sigset_t *set);
void dispatch_signals(struct proc *td);
int signals_init(struct proc *td);

/* Sigset functions */
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);

/* Default handlers */
void sigfpe_default(int signo);
void sigkill_default(int signo);
void sigsegv_default(int signo);
void sigterm_default(int signo);
#endif  /* _KERNEL */
#endif  /* !_SYS_SIGNAL_H_ */

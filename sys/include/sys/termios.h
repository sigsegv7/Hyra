/*
 * Copyright (c) 2023-2024 Ian Marco Moffett and the Osmora Team.
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

#ifndef _SYS_TERMIOS_H_
#define _SYS_TERMIOS_H_

#if defined(_KERNEL)
#include <sys/types.h>
#else
#include <stdint.h>
#endif

#define NCCS 20

/*
 * Output flags - software output processing
 */
#define OPOST       0x00000001U         /* Enable output processing */
#if defined(_KERNEL) || defined(_HYRA_SOURCE)
#define OCRNL       0x00000002U         /* Map NL to CR-NL */
#define ORBUF       0x00000004U         /* Buffer bytes */
#endif      /* defined(_KERNEL) || defined(_HYRA_SOURCE) */

typedef uint32_t tcflag_t;
typedef uint32_t speed_t;
typedef uint8_t  cc_t;

struct termios {
    tcflag_t    c_iflag;        /* Input flags */
    tcflag_t    c_oflag;        /* Output flags */
    tcflag_t    c_cflag;        /* Control flags */
    tcflag_t    c_lflag;        /* Local flags */
    cc_t        c_cc[NCCS];     /* Control chars */
    int         c_ispeed;       /* Input speed */
    int         c_ospeed;       /* Output speed */
};

#endif  /* !_SYS_TERMIOS_H_ */

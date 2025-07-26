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

#ifndef _TERMIOS_H_
#define _TERMIOS_H_

/*
 * c_iflag: Input flags
 */
#define ISTRIP 0x00000001   /* Strip char */
#define ICRNL  0x00000002   /* Map CR to NL */
#define BRKINT 0x00000004   /* Signal interrupt on break */
#define IGNBRK 0x00000008   /* Ignore break condition */
#define IGNCR  0x00000010   /* Ignore CR */
#define IGNPAR 0x00000020   /* Ignore chars with parity errors */
#define INCLR  0x00000040   /* Map NL to CR */
#define INPCK  0x00000080   /* Enable input parity check */
#define IXANY  0x00000100   /* Enable any char to restart output */
#define IXOFF  0x00000200   /* Enable start/stop control */
#define PARMRK 0x00000400   /* Mark parity errors */

/*
 * c_oflag: Output flags
 */
#define OPOST  0x00000001   /* Post-process output */
#define ONLCR  0x00000002   /* Map NL to CR-NL on output */
#define OCRNL  0x00000004   /* Map CR to NL on output */
#define ONOCR  0x00000008   /* Map CR to output at col 0 */
#define ONLRET 0x00000010   /* NL performs CR function */
#define OFILL  0x00000020   /* Use fill chars for delay */
#define NLDLY  0x00000040   /* Select newline type  */
#define CRDLY  0x00000080   /* Select carriage-return delays */
#define TABDLY 0x00000100   /* Select horizontal-tab delays */
#define BSDLY  0x00000200   /* Select backspace delays */
#define VTDLY  0x00000400   /* Select veritcal tab delays */
#define FFDLY  0x00000800   /* Select form-feed delays */

#define NCCS 20

typedef unsigned int cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

struct termios {
    tcflag_t c_iflag;   /* Input flags */
    tcflag_t c_oflag;   /* Output flags */
    tcflag_t c_cflag;   /* Control flags */
    tcflag_t c_lflag;   /* Local flags */
    cc_t c_cc[NCCS];
};

#endif  /* _TERMIOS_H_ */

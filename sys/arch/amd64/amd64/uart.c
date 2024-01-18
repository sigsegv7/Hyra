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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <machine/uart.h>
#include <machine/io.h>

#define UART_COM1 0x3F8

#define UART_PORTNO(OFFSET) (UART_COM1 + 1)

static bool
uart8250_transmit_empty(void)
{
    return __TEST(UART_PORTNO(5), __BIT(5));
}

void
uart8250_write(char byte)
{
    while (!uart8250_transmit_empty());
    outb(UART_COM1, byte);
}

int
uart8250_try_init(void)
{
    /* Disable interrutps */
    outb(UART_PORTNO(1), 0x00);

    /* Enable DLAB to set divisor latches */
    outb(UART_PORTNO(3), 0x80);

    /* Set to 38400 baud via divisor latches (DLL and DLH)*/
    outb(UART_PORTNO(0), 0x03);
    outb(UART_PORTNO(1), 0x00);

    /*
     * Set Data Word Length to 8 bits...
     *
     * XXX: Our write does not preserve the DLAB bit...
     *      We want to clear it to make the baud latches
     *      readonly
     */
    outb(UART_PORTNO(3), 0x03);

    /*
     * We want FIFO to be enabled, and want to clear the
     * TX/RX queues. We also want to set the interrupt
     * watermark at 14 bytes.
     */
    outb(UART_PORTNO(2), 0xC7);

    /*
     * Enable auxiliary output 2 (used as interrupt line) and
     * mark data terminal ready.
     */
    outb(UART_PORTNO(4), 0x0B);

    /* Enable interrupts */
    outb(UART_PORTNO(1), 0x01);

    /* Put chip in loopback mode... test chip w/ test byte */
    outb(UART_PORTNO(4), 0x1E);
    outb(UART_PORTNO(0), 0xAE);
    if (inb(UART_PORTNO(0) != 0xAE)) {
        /* Not the same byte, something is wrong */
        return 1;
    }
    return 0;
}

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

#include <sys/param.h>
#include <machine/uart.h>
#include <machine/pio.h>

#define UART_COM1 0x3F8
#define UART_REG(OFFSET) (UART_COM1 + OFFSET)

static inline bool
uart_transmit_empty(void)
{
    return ISSET(UART_REG(5), BIT(5));
}

void
uart_write(char byte)
{
    while (!uart_transmit_empty());
    outb(UART_COM1, byte);
}

int
uart_init(void)
{
    /* Disable interrupts */
    outb(UART_REG(1), 0x00);

    /* Set DLAB to set baud rate */
    outb(UART_REG(3), 0x80);

    /* Set rate to 57600 baud */
    outb(UART_REG(0), 0x02);
    outb(UART_REG(1), 0x00);

    /* Set data word length to 8 bits and clear DLAB */
    outb(UART_REG(3), 0x03);

    /* Disable FIFOs for now... (TODO: Use them) */
    outb(UART_REG(2), 0x00);

    /* Test chip with loopback mode */
    outb(UART_REG(4), 0x10);
    outb(UART_REG(0), 0xF0);
    if (inb(UART_REG(0) != 0xF0))
        return -1;

    /*
     * Mark the data terminal ready and clear
     * loopback mode.
     */
    outb(UART_REG(4), 0x01);
    return 0;
}

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

/* Channel port numbers */
#define UART_COM1 0x3F8
#define UART_COM2 0x2F8
#define UART_COM3 0x3E8
#define UART_COM4 0x2E8
#define UART_COM5 0x5F8
#define UART_COM6 0x4F8
#define UART_COM7 0x5E8
#define UART_COM8 0x4E8

/* Register offsets */
#define UART_REG(OFFSET) (UART_COM1 + OFFSET)
#define UART_REG_FCR UART_REG(2) /* FIFO Control Register */
#define UART_REG_LCR UART_REG(3) /* Line Control Register */
#define UART_REG_MCR UART_REG(4) /* MODEM Control Register */
#define UART_REG_LSR UART_REG(5) /* Line Status Register */
#define UART_REG_MSR UART_REG(6) /* MODEM Status Register */
#define UART_REG_SR  UART_REG(7) /* Scratch Register */

/* Registers when LCR.DLAB=0 */
#define UART_REG_RBR UART_REG(0) /* Receiver Buffer Register */
#define UART_REG_THR UART_REG(0) /* Transmitter Holding Register */
#define UART_REG_IER UART_REG(1) /* Interrupt Enable Register */

/* Registers when LCR.DLAB=1 */
#define UART_REG_DLL UART_REG(0) /* Divisor Latch Low */
#define UART_REG_DLH UART_REG(1) /* Divisor Latch High */

#define UART_LCR_WLS0 BIT(0) /* Word Length Select Bit 0 */
#define UART_LCR_WLS1 BIT(1) /* Word Length Select Bit 1 */
#define UART_LCR_DLAB BIT(7) /* Divisor Latch Access Bit*/

#define UART_MCR_DTR  BIT(0) /* Data Terminal Ready*/
#define UART_MCR_LOOP BIT(4) /* Loop */

#define UART_LSR_THRE BIT(5) /* Transmitter Holding Register */

#define UART_DIVISOR(RATE) (115200 / RATE)

static inline bool
uart_transmit_empty(void)
{
    return ISSET(UART_REG_LSR, UART_LSR_THRE);
}

void
uart_write(char byte)
{
    while (!uart_transmit_empty());
    outb(UART_REG_THR, byte);
}

int
uart_init(void)
{
    /* Disable interrupts */
    outb(UART_REG_IER, 0x00);

    /* Set DLAB to set baud rate */
    outb(UART_REG_LCR, UART_LCR_DLAB);

    /* Set speed to 57600 baud */
    outb(UART_REG_DLL, UART_DIVISOR(57600));
    outb(UART_REG_IER, 0x00);

    /* Set word size to 8 bits and clear DLAB */
    outb(UART_REG_LCR, UART_LCR_WLS0 | UART_LCR_WLS1);

    /* Disable FIFOs for now... (TODO: Use them) */
    outb(UART_REG_FCR, 0x00);

    /* Test chip in loopback mode */
    outb(UART_REG_MCR, UART_MCR_LOOP);
    outb(UART_REG_THR, 0xF0);
    if (inb(UART_REG_RBR != 0xF0))
        return -1;

    /*
     * Mark the data terminal ready and clear
     * loopback mode.
     */
    outb(UART_REG_MCR, UART_MCR_DTR);
    return 0;
}
 
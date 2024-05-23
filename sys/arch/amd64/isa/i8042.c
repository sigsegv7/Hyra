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

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/driver.h>
#include <sys/errno.h>
#include <sys/spinlock.h>
#include <sys/tty.h>
#include <sys/intr.h>
#include <machine/ioapic.h>
#include <machine/idt.h>
#include <machine/io.h>
#include <machine/lapic.h>
#include <machine/hpet.h>
#include <machine/sysvec.h>
#include <machine/isa/i8042regs.h>

__MODULE_NAME("i8042");
__KERNEL_META("$Hyra$: i8042.c, Ian Marco Moffett, "
              "i8042 PS/2 driver");

static struct spinlock data_lock;
static bool shift_key = false;
static bool capslock = false;
static bool capslock_released = true;
static struct intr_info *irq_info;

static int dev_send(bool aux, uint8_t data);

static char keytab[] = {
    '\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', '\0', 'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.',  '/', '\0', '\0', '\0', ' '
};

static char keytab_shift[] = {
    '\0', '\0', '!', '@', '#',  '$', '%', '^',  '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R',  'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\n',  '\0', 'A', 'S',  'D', 'F', 'G', 'H',
    'J', 'K', 'L', ':', '\"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>',  '?', '\0', '\0', '\0', ' '
};

static char keytab_caps[] = {
    '\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-','=', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '[', ']', '\n', '\0', 'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L', ';', '\'', '`', '\0', '\\', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', ',', '.', '/', '\0', '\0', '\0', ' '
};

static void
kbd_set_leds(uint8_t mask)
{
    dev_send(false, 0xED);
    dev_send(false, mask);
}

/*
 * Convert scancode to character
 *
 * @sc: Scancode
 * @chr: Character output
 *
 * Returns 0 when a char is given back.
 */
static int
scancode_to_chr(uint8_t sc, char *chr)
{
    bool release = __TEST(sc, __BIT(7));

    switch (sc) {
    /* Capslock pressed */
    case 0x3A:
        /*
         * If we are holding down caps-lock, we do not
         * want a stream of presses that constantly cause
         * it to toggle, only toggle if released then pushed
         * again.
         */
        if (!capslock_released)
            return -EAGAIN;

        capslock_released = false;
        capslock = !capslock;

        if (!capslock) {
            kbd_set_leds(0);
        } else {
            kbd_set_leds(I8042_LED_CAPS);
        }
        return -EAGAIN;
    /* Capslock released */
    case 0xBA:
        capslock_released = true;
        return -EAGAIN;
    /* Shift */
    case 0x36:
    case 0xAA:
    case 0x2A:
    case 0xB6:
        if (!release) {
            shift_key = true;
        } else {
            shift_key = false;
        }
        return -EAGAIN;
    }

    if (release) {
        return -EAGAIN;
    }

    if (capslock) {
        *chr = keytab_caps[sc];
        return 0;
    }

    if (shift_key) {
        *chr = keytab_shift[sc];
        return 0;
    }

    *chr = keytab[sc];
    return 0;
}

/*
 * Write to an i8042 register.
 *
 * @port: I/O port
 * @val: Value to write
 */
static void
i8042_write(uint16_t port, uint8_t val)
{
    while (__TEST(I8042_STATUS, I8042_IBUF_FULL));
    outb(port, val);
}

/*
 * Read the i8042 config register
 */
static uint8_t
i8042_read_conf(void)
{
    i8042_write(I8042_COMMAND, I8042_GET_CONFB);
    return inb(I8042_DATA);
}

/*
 * Write the i8042 config register
 */
static void
i8042_write_conf(uint8_t value)
{
    i8042_write(I8042_COMMAND, I8042_SET_CONFB);
    i8042_write(I8042_DATA, value);
}

/*
 * Flush the data register until it is empty.
 */
static void
i8042_drain(void)
{
    spinlock_acquire(&data_lock);
    while (__TEST(inb(I8042_STATUS), I8042_OBUF_FULL)) {
        inb(I8042_DATA);
    }

    spinlock_release(&data_lock);
}

/*
 * Send a data to a device
 *
 * @aux: If true, send to aux device (mouse)
 * @data: Data to send.
 */
static int
dev_send(bool aux, uint8_t data)
{
    if (aux) {
        i8042_write(I8042_COMMAND, I8042_PORT1_SEND);
    }

    for (;;) {
        if (!__TEST(inb(I8042_STATUS), I8042_IBUF_FULL)) {
            break;
        }
    }

    i8042_write(I8042_DATA, data);
    inb(0x80);      /* Waste a cycle */
    return inb(I8042_DATA);
}

__attr(interrupt)
static void
kb_isr(void *sf)
{
    uint8_t data;
    char c;

    spinlock_acquire(&irq_info->lock);
    ++irq_info->count;
    spinlock_release(&irq_info->lock);

    spinlock_acquire(&data_lock);
    while (__TEST(inb(I8042_STATUS), I8042_OBUF_FULL)) {
        data = inb(I8042_DATA);
        if (scancode_to_chr(data, &c) == 0) {
            tty_putc(&g_root_tty, c, TTY_SOURCE_DEV);
        }
    }
    spinlock_release(&data_lock);
    lapic_send_eoi();
}

static int
i8042_init(void)
{
    uint8_t conf;

    /*
     * Disable the ports and drain the output buffer
     * to avoid interferance with the initialization
     * process.
     */
    i8042_write(I8042_COMMAND, I8042_DISABLE_PORT0);
    i8042_write(I8042_COMMAND, I8042_DISABLE_PORT1);
    i8042_drain();

    /* Disable aux data streaming */
    dev_send(true, I8042_AUX_DISABLE);
    i8042_drain();

    /* Setup kbd interrupts */
    idt_set_desc(SYSVEC_PCKBD, IDT_INT_GATE_FLAGS, (uintptr_t)kb_isr, 0);
    ioapic_set_vec(1, SYSVEC_PCKBD);
    ioapic_irq_unmask(1);

    /* Register the interrupt */
    irq_info = intr_info_alloc("IOAPIC", "i8042");
    irq_info->affinity = 0;
    intr_register(irq_info);

    /* Setup config bits */
    conf = i8042_read_conf();
    conf |= I8042_PORT0_INTR;
    conf &= ~I8042_PORT1_INTR;
    i8042_write_conf(conf);

    /* Enable the keyboard */
    i8042_write(I8042_COMMAND, I8042_ENABLE_PORT0);

    /*
     * It seems one I/O bus cycle isn't enough to wait for data
     * to accumulate on some machines... Give it around 50ms then
     * drain the output buffer.
     */
    hpet_msleep(50);
    i8042_drain();
    return 0;
}

DRIVER_EXPORT(i8042_init);

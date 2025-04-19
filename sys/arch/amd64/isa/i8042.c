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

#include <sys/types.h>
#include <sys/driver.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/spinlock.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/queue.h>
#include <dev/acpi/acpi.h>
#include <dev/timer.h>
#include <dev/cons/cons.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/isa/i8042var.h>
#include <machine/isa/spkr.h>
#include <machine/idt.h>
#include <machine/ioapic.h>
#include <machine/intr.h>
#include <machine/lapic.h>
#include <machine/cdefs.h>
#include <string.h>
#include <assert.h>

#define KEY_REP_MAX 2

#define pr_trace(fmt, ...) kprintf("i8042: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define IO_NOP() inb(0x80)

static struct spinlock data_lock;
static struct spinlock isr_lock;
static bool shift_key = false;
static bool capslock = false;
static bool capslock_released = true;
static uint16_t quirks = 0;
static struct proc polltd;
static struct timer tmr;
static bool is_init = false;

static int dev_send(bool aux, uint8_t data);
static int i8042_kb_getc(uint8_t sc, char *chr);
static void i8042_drain(void);

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
 * Poll the i8042 status register
 *
 * @bits: Status bits.
 * @pollset: True to poll if set
 */
static int
i8042_statpoll(uint8_t bits, bool pollset)
{
    size_t usec_start, usec;
    size_t elapsed_msec;
    uint8_t val;
    bool tmp;

    usec_start = tmr.get_time_usec();
    for (;;) {
        val = inb(I8042_STATUS);
        tmp = (pollset) ? ISSET(val, bits) : !ISSET(val, bits);
        usec = tmr.get_time_usec();
        elapsed_msec = (usec - usec_start) / 1000;

        IO_NOP();

        /* If tmp is set, the register updated in time */
        if (tmp) {
            break;
        }

        /* Exit with an error if we timeout */
        if (elapsed_msec > I8042_DELAY) {
            return -ETIME;
        }
    }

    return val;
}

/*
 * Drain i8042 internal data registers.
 */
static void
i8042_drain(void)
{
    spinlock_acquire(&data_lock);
    while (ISSET(inb(I8042_STATUS), I8042_OBUFF)) {
        inb(I8042_DATA);
    }
    spinlock_release(&data_lock);
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
    i8042_statpoll(I8042_IBUFF, false);
    outb(port, val);
}

/*
 * Read the i8042 config register
 */
static uint8_t
i8042_read_conf(void)
{
    i8042_drain();
    i8042_write(I8042_CMD, I8042_GET_CONFB);
    i8042_statpoll(I8042_OBUFF, true);
    return inb(I8042_DATA);
}

/*
 * Write the i8042 config register
 */
static void
i8042_write_conf(uint8_t value)
{
    i8042_drain();
    i8042_statpoll(I8042_IBUFF, false);
    i8042_write(I8042_CMD, I8042_SET_CONFB);
    i8042_statpoll(I8042_IBUFF, false);
    i8042_write(I8042_DATA, value);
    i8042_drain();
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
        i8042_write(I8042_CMD, I8042_PORT1_SEND);
    }

    i8042_statpoll(I8042_IBUFF, false);
    i8042_write(I8042_DATA, data);
    i8042_statpoll(I8042_OBUFF, true);
    return inb(I8042_DATA);
}

void
i8042_kb_event(void)
{
    struct cpu_info *ci;
    struct cons_input input;
    uint8_t data;
    char c;

    spinlock_acquire(&isr_lock);
    ci = this_cpu();
    ci->irq_mask |= CPU_IRQ(1);
    data = inb(I8042_DATA);

    if (i8042_kb_getc(data, &c) != 0) {
        /* No data useful */
        goto done;
    }
    input.scancode = data;
    input.chr = c;
    cons_ibuf_push(&g_root_scr, input);
done:
    ci->irq_mask &= CPU_IRQ(1);
    spinlock_release(&isr_lock);
    lapic_eoi();
}

static void
i8042_en_intr(void)
{
    uint8_t conf;
    int vec;

    pr_trace("ENTER -> i8042_en_intr\n");
    i8042_write(I8042_CMD, I8042_DISABLE_PORT0);
    pr_trace("port 0 disabled\n");

    vec = intr_alloc_vector("i8042-kb", IPL_BIO);
    idt_set_desc(vec, IDT_INT_GATE, ISR(i8042_kb_isr), IST_HW_IRQ);
    ioapic_set_vec(KB_IRQ, vec);
    ioapic_irq_unmask(KB_IRQ);
    pr_trace("irq 1 -> vec[%x]\n", vec);

    /* Setup config bits */
    conf = i8042_read_conf();
    conf |= I8042_PORT0_INTR;
    conf &= ~I8042_PORT1_INTR;
    i8042_write_conf(conf);
    pr_trace("conf written\n");

    i8042_write(I8042_CMD, I8042_ENABLE_PORT0);
    pr_trace("port 0 enabled\n");
}

static void
esckey_reboot(void)
{
    syslock();
    kprintf("** Machine going down for a reboot");

    for (size_t i = 0; i < 3; ++i) {
        kprintf(OMIT_TIMESTAMP ".");
        tmr.msleep(1000);
    }

    cpu_reboot(0);
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
i8042_kb_getc(uint8_t sc, char *chr)
{
    bool release = ISSET(sc, BIT(7));

    switch (sc) {
    /* Left alt [press] */
    case 0x38:
        esckey_reboot();
        break;
    /* Caps lock [press] */
    case 0x3A:
        /*
         * In case we are holding the caps lock button down,
         * we don't want it to be spam toggled as that would
         * be pretty strange looking and probably annoying.
         */
        if (!capslock_released) {
            return -EAGAIN;
        }

        capslock_released = false;
        capslock = !capslock;

        if (!capslock) {
            kbd_set_leds(0);
        } else {
            kbd_set_leds(I8042_LED_CAPS);
        }
        return -EAGAIN;
    /* Caps lock [release] */
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

static void
i8042_sync_loop(void)
{
    /* Wake up the bus */
    outb(I8042_DATA, 0x00);
    i8042_drain();

    for (;;) {
        i8042_sync();
        md_pause();
    }
}

/*
 * Grabs a key from the keyboard, used typically
 * for syncing the machine however can be used
 * to bypass IRQs in case of buggy EC.
 */
void
i8042_sync(void)
{
    static struct spinlock lock;
    struct cons_input input;
    uint8_t data;
    char c;

    if (spinlock_try_acquire(&lock)) {
        return;
    }

    if (ISSET(quirks, I8042_HOSTILE) && is_init) {
        if (i8042_statpoll(I8042_OBUFF, true) < 0) {
            /* No data ready */
            goto done;
        }
        data = inb(I8042_DATA);

        if (i8042_kb_getc(data, &c) == 0) {
            input.scancode = data;
            input.chr = c;
            cons_ibuf_push(&g_root_scr, input);
        }
    }
done:
    spinlock_release(&lock);
}

void
i8042_quirk(int mask)
{
    quirks |= mask;
}

static int
i8042_init(void)
{
    /* Try to request a general purpose timer */
    if (req_timer(TIMER_GP, &tmr) != TMRR_SUCCESS) {
        pr_error("failed to fetch general purpose timer\n");
        return -ENODEV;
    }

    /* Ensure it has get_time_usec() */
    if (tmr.get_time_usec == NULL) {
        pr_error("general purpose timer has no get_time_usec()\n");
        return -ENODEV;
    }

    /* We also need msleep() */
    if (tmr.msleep == NULL) {
        pr_error("general purpose timer has no msleep()\n");
        return -ENODEV;
    }

    /*
     * On some thinkpads, e.g., the T420s, the EC implementing
     * the i8042 logic likes to play cop and throw NMIs at us
     * for anything we do e.g., config register r/w, IRQs,
     * etc... As of now, treat the i8042 like a fucking bomb
     * if this bit is set.
     */
    if (strcmp(acpi_oemid(), "LENOVO") == 0) {
        quirks |= I8042_HOSTILE;
        pr_trace("lenovo device, assuming hostile\n");
        pr_trace("disabling irq 1, polling as fallback\n");
        fork1(&polltd, 0, i8042_sync_loop, NULL);
    }

    if (!ISSET(quirks, I8042_HOSTILE)) {
        /* Enable interrupts */
        i8042_drain();
        i8042_en_intr();
    }

    if (dev_send(false, 0xFF) == 0xFC) {
        pr_error("kbd self test failure\n");
        return -EIO;
    }

    is_init = true;
    return 0;
}

DRIVER_EXPORT(i8042_init);

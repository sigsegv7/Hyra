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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ascii.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <dev/video/fbdev.h>
#include <dev/cons/font.h>
#include <dev/cons/cons.h>
#include <fs/devfs.h>
#include <fs/ctlfs.h>
#include <vm/dynalloc.h>
#include <string.h>

#define HIDE_CURSOR(SCR) \
    cons_draw_cursor((SCR), (SCR)->bg)

#define SHOW_CURSOR(SCR) \
    cons_draw_cursor((SCR), rgb_invert((SCR)->bg))

/* Console background from kconf */
#if defined(__CONSOLE_BG)
#define CONSOLE_BG __CONSOLE_BG
#else
#define CONSOLE_BG 0x000000
#endif /* __CONSOLE_BG */

/* Console foreground from kconf */
#if defined(__CONSOLE_FG)
#define CONSOLE_FG __CONSOLE_FG
#else
#define CONSOLE_FG 0x00AA00
#endif  /* __CONSOLE_FG */

struct cons_screen g_root_scr = {0};
static struct cdevsw cons_cdevsw;
static struct ctlops cons_feat_ctl;
static struct ctlops cons_attr_ctl;

static void cons_draw_cursor(struct cons_screen *scr, uint32_t color);
static int cons_handle_special(struct cons_screen *scr, char c);

static uint32_t
rgb_invert(uint32_t rgb)
{
    uint8_t r, g, b;
    uint32_t ret;

    r = (rgb >> 16) & 0xFF;
    g = (rgb >> 8) & 0xFF;
    b = rgb & 0xFF;

    ret = (255 - r) << 16;
    ret |= (255 - g) << 8;
    ret |= 255 - b;
    return ret;
}


/*
 * Render a character onto the screen.
 *
 * @scr: Screen to render to.
 * @c: Char to render.
 * @x: X position of char.
 * @y: Y position of char.
 */
static void
cons_draw_char(struct cons_screen *scr, struct cons_char ch)
{
    size_t idx;
    uint32_t x, y;
    const uint8_t *glyph;

    if (scr->fb_mem == NULL) {
        return;
    }

    glyph = &CONS_FONT[(int)ch.c*16];
    x = ch.x;
    y = ch.y;

    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        idx = fbdev_get_index(&scr->fbdev, x + (FONT_WIDTH - 1), y + cy);
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            scr->fb_mem[idx--] = ISSET(glyph[cy], BIT(cx)) ? ch.fg : ch.bg;
        }
    }
}

/*
 * Internal helper - flush console row
 *
 * @row: Row to flush.
 */
static int
cons_flush_row(struct cons_screen *scr, uint32_t row)
{
    struct cons_buf *bp;
    struct cons_char cc;

    bp = scr->ob[row];
    if (ISSET(bp->flags, CONS_BUF_CLEAN)) {
        return -EIO;
    }

    for (int j = 0; j < bp->len; ++j) {
        if (cons_obuf_pop(bp, &cc) != 0) {
            continue;
        }

        cons_draw_char(scr, cc);
        bp->flags |= CONS_BUF_CLEAN;
    }

    return 0;
}

/*
 * Internal helper - flush console
 *
 */
static int
cons_flush(struct cons_screen *scr)
{
    for (int i = 0; i < scr->nrows; ++i) {
        cons_flush_row(scr, i);
    }
    return 0;
}

/*
 * Handle a special character (e.g  "\t", "\n", etc)
 *
 * @scr: Screen to handle this on.
 * @c: Char to handle.
 *
 * Returns 0 if handled, otherwise -1.
 */
static int
cons_handle_special(struct cons_screen *scr, char c)
{
    struct cons_buf *bp;

    if (scr->ch_col >= scr->ncols - 20) {
        scr->ch_col = 0;
        cons_handle_special(scr, '\n');
    }

    switch (c) {
    case ASCII_HT:
        HIDE_CURSOR(scr);
        scr->curs_col += 4;
        scr->ch_col += 4;
        if (scr->ch_col >= scr->ncols - 1) {
           cons_handle_special(scr, '\n');
        }
        SHOW_CURSOR(scr);
        return 0;
    case ASCII_NUL:
        return 0;
    case ASCII_BS:
        bp = scr->ob[scr->ch_row];
        if (bp->head > bp->tail) {
            --bp->head;
        }

        HIDE_CURSOR(scr);
        if (scr->ch_col > 0 && scr->curs_col > 0) {
            --scr->ch_col;
            --scr->curs_col;
        }
        SHOW_CURSOR(scr);
        return 0;
    case ASCII_LF:
        /* Are we past screen width? */
        if (scr->ch_row >= scr->nrows - 1) {
            cons_clear_scr(scr, scr->bg);
            return 0;
        }

        HIDE_CURSOR(scr);

        /* Make a newline */
        cons_flush(scr);
        ++scr->ch_row;
        scr->ch_col = 0;
        cons_flush(scr);

        /* Update cursor */
        scr->curs_row += 1;
        scr->curs_col = 0;
        SHOW_CURSOR(scr);
        return 0;
    case ASCII_FF:
        /*
         * Fuck what they say, this is now the
         * "flush" byte ::)
         */
        cons_flush(scr);
        return 0;
    }

    return -1;
}

static void
cons_draw_cursor(struct cons_screen *scr, uint32_t color)
{
    struct console_feat *featp;
    size_t idx;

    featp = &scr->feat;
    if (!featp->show_curs) {
        color = scr->bg;
    }

    /* Past screen width? */
    if (scr->curs_col >= scr->ncols) {
        scr->curs_col = 0;
        scr->curs_row++;
    }

    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        idx = fbdev_get_index(&scr->fbdev, scr->curs_col * FONT_WIDTH, (scr->curs_row * FONT_HEIGHT) + cy);
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            scr->fb_mem[idx++] = color;
        }
    }
}

/*
 * Clear the screen.
 *
 * @scr: Screen to clear.
 * @bg: Color to clear it to.
 */
void
cons_clear_scr(struct cons_screen *scr, uint32_t bg)
{
    struct fbdev fbdev = scr->fbdev;

    cons_flush(scr);
    HIDE_CURSOR(scr);

    scr->ch_col = 0;
    scr->ch_row = 0;
    scr->curs_col = 0;
    scr->curs_row = 0;

    for (size_t i = 0; i < fbdev.height * fbdev.pitch; ++i) {
        scr->fb_mem[i] = bg;
    }

    SHOW_CURSOR(scr);

}

/*
 * Quickly put a character on the screen.
 * XXX: Does not acquire the screen's lock or show/hide the cursor.
 *
 * @scr: Screen.
 * @c: Character to draw.
 */
static void
cons_fast_putch(struct cons_screen *scr, char c)
{
    struct cons_char cc;
    struct cons_buf *bp;
    int ansi;

    ansi = ansi_feed(&scr->ansi_s, c);
    if (ansi > 0) {
        c = ASCII_NUL;
    } else if (ansi < 0) {
        c = ASCII_NUL;
    }

    /* Handle specials */
    if (cons_handle_special(scr, c) == 0) {
        return;
    }

    /* Create a new character */
    cc.c = c;
    cc.fg = scr->fg;
    cc.bg = scr->bg;
    cc.x = scr->ch_col * FONT_WIDTH;
    cc.y = scr->ch_row * FONT_HEIGHT;

    /* Push our new character */
    bp = scr->ob[scr->ch_row];
    bp->flags &= ~CONS_BUF_CLEAN;
    cons_obuf_push(bp, cc);
    ++scr->ch_col;

    /* Check screen bounds */
    if (cc.x >= (scr->ncols * FONT_WIDTH) - 1) {
        scr->ch_col = 0;
        ++scr->ch_row;
    }

    ++scr->curs_col;
    if (scr->curs_col > scr->ncols - 1) {
        scr->curs_col = 0;
        if (scr->curs_row < scr->nrows)
            ++scr->curs_row;
    }
}

/*
 * Character device function.
 */
static int
dev_write(dev_t dev, struct sio_txn *sio, int flags)
{
    cons_attach();
    cons_putstr(&g_root_scr, sio->buf, sio->len);
    cons_flush(&g_root_scr);
    return sio->len;
}

/*
 * Character device function.
 */
static int
dev_read(dev_t dev, struct sio_txn *sio, int flags)
{
    struct cons_input input;
    uint8_t *p;
    int retval;
    size_t n;

    p = (uint8_t *)sio->buf;
    n = sio->len;

    /* Must be a power of two */
    if ((n & 1) != 0) {
        return -EFAULT;
    }

    retval = cons_ibuf_pop(&g_root_scr, &input);
    if (retval < 0) {
        return -EAGAIN;
    }

    cons_attach();
    spinlock_acquire(&g_root_scr.lock);
    for (;;) {
        /* Buffer too small */
        if (n == 0) {
            break;
        }

        *p++ = input.chr;
        *p++ = input.scancode;
        n -= 2;

        /* Try to get the next byte */
        retval = cons_ibuf_pop(&g_root_scr, &input);
        if (retval < 0) {
            break;
        }
    }
    spinlock_release(&g_root_scr.lock);
    return sio->len;
}

static int
cons_init_bufs(struct cons_screen *scr)
{
    struct cons_buf *bp;
    size_t ob_len;

    scr->ib = cons_new_buf(CONS_BUF_INPUT, scr->ncols);
    if (g_root_scr.ib == NULL) {
        panic("out of memory\n");
    }

    ob_len = sizeof(*scr->ob) * scr->nrows;
    scr->ob = dynalloc(ob_len);

    /* Allocate all output buffers per line */
    for (size_t i = 0; i < scr->nrows; ++i) {
        bp = cons_new_buf(CONS_BUF_OUTPUT, scr->ncols);
        if (bp == NULL) {
            panic("out of memory\n");
        }
        bp->flags |= CONS_BUF_CLEAN;
        scr->ob[i] = bp;
    }

    return 0;
}

static int
ctl_feat_read(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    struct cons_screen *scr = &g_root_scr;
    struct console_feat *featp;

    if (sio->buf == NULL || sio->len == 0) {
        return -EINVAL;
    }

    featp = &scr->feat;
    if (sio->len > sizeof(*featp)) {
        sio->len = sizeof(*featp);
    }

    memcpy(sio->buf, featp, sio->len);
    return sio->len;
}

static int
ctl_feat_write(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    struct cons_screen *scr = &g_root_scr;
    struct console_feat *featp, oldfeat;

    featp = &scr->feat;
    oldfeat = *featp;
    if (sio->len > sizeof(*featp)) {
        sio->len = sizeof(*featp);
    }

    memcpy(featp, sio->buf, sio->len);

    /*
     * If we are suddenly trying to reset the cursor
     * status, redraw it.
     */
    if (featp->show_curs != oldfeat.show_curs) {
        if (featp->show_curs == 0) {
            HIDE_CURSOR(scr);
        } else {
            SHOW_CURSOR(scr);
        }
    }
    return sio->len;
}

static int
ctl_attr_read(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    struct cons_screen *scr = &g_root_scr;
    struct console_attr *attrp;

    if (sio->buf == NULL || sio->len == 0) {
        return -EINVAL;
    }

    attrp = &scr->attr;
    if (sio->len > sizeof(*attrp)) {
        sio->len = sizeof(*attrp);
    }

    memcpy(sio->buf, attrp, sio->len);
    return sio->len;
}

static int
ctl_attr_write(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    struct cons_screen *scr = &g_root_scr;
    struct console_attr *attrp;

    attrp = &scr->attr;
    if (sio->len > sizeof(*attrp)) {
        sio->len = sizeof(*attrp);
    }

    spinlock_acquire(&scr->lock);
    HIDE_CURSOR(scr);
    memcpy(attrp, sio->buf, sio->len);

    /* Clip the x/y positions */
    if (attrp->cursor_x >= scr->ncols)
        attrp->cursor_x = scr->ncols - FONT_WIDTH;
    if (attrp->cursor_y >= scr->nrows)
        attrp->cursor_y = scr->nrows - FONT_HEIGHT;

    /* Update cursor */
    scr->curs_col = attrp->cursor_x;
    scr->curs_row = attrp->cursor_y;
    scr->ch_col = attrp->cursor_x;
    scr->ch_row = attrp->cursor_y;
    SHOW_CURSOR(scr);

    spinlock_release(&scr->lock);
    return sio->len;
}

/*
 * Detach the currently running process from the
 * console.
 */
int
cons_detach(void)
{
    struct cons_screen *scr;

    scr = &g_root_scr;
    if (scr->atproc == NULL) {
        return 0;
    }
    if (scr->atproc_lock == NULL) {
        return 0;
    }

    scr = &g_root_scr;
    scr->atproc = NULL;
    mutex_release(scr->atproc_lock);
    return 0;
}

/*
 * Attach the current process to the
 * console.
 */
int
cons_attach(void)
{
    struct cons_screen *scr;
    struct proc *td, *atproc;

    td = this_td();
    if (td == NULL) {
        return -1;
    }

    scr = &g_root_scr;
    if (scr->atproc_lock == NULL) {
        return 0;
    }

    scr = &g_root_scr;
    atproc = scr->atproc;

    if (atproc != NULL) {
        if (atproc->pid == td->pid) {
            return 0;
        }

        /*
         * Do not release this here as we want
         * any other process that tries to attach
         * to wait.
         */
        mutex_acquire(scr->atproc_lock, 0);
    }

    scr->atproc = td;
    return 0;
}

/*
 * Reset console color.
 */
void
cons_reset_color(struct cons_screen *scr)
{
    g_root_scr.fg = CONSOLE_FG;
    g_root_scr.bg = CONSOLE_BG;
}

void
cons_update_color(struct cons_screen *scr, uint32_t fg, uint32_t bg)
{
    scr->fg = fg;
    scr->bg = bg;
}

void
cons_reset_cursor(struct cons_screen *scr)
{
    HIDE_CURSOR(scr);
    scr->ch_col = 0;
    scr->ch_row = 0;
    scr->curs_col = 0;
    scr->curs_row = 0;
    SHOW_CURSOR(scr);
}

/*
 * Put a character on the screen.
 *
 * @scr: Screen.
 * @c: Character to draw.
 */
int
cons_putch(struct cons_screen *scr, char c)
{
    spinlock_acquire(&scr->lock);
    HIDE_CURSOR(scr);

    cons_fast_putch(scr, c);

    SHOW_CURSOR(scr);
    spinlock_release(&scr->lock);
    return 0;
}

/*
 * Put a string on the screen.
 *
 * @scr: Screen.
 * @s: String to draw.
 * @l: Length of s.
 */
int
cons_putstr(struct cons_screen *scr, const char *s, size_t len)
{
    const char *p = s;

    spinlock_acquire(&scr->lock);
    HIDE_CURSOR(scr);

    while (len--) {
        cons_fast_putch(scr, *p);
        ++p;
    }

    SHOW_CURSOR(scr);
    spinlock_release(&scr->lock);
    return 0;
}

void
cons_init(void)
{
    struct fbdev fbdev = fbdev_get();
    struct console_feat *featp;

    featp = &g_root_scr.feat;
    featp->ansi_esc = 1;
    featp->show_curs = 1;
    g_root_scr.ch_col = 0;
    g_root_scr.ch_row = 0;
    g_root_scr.fg = CONSOLE_FG;
    g_root_scr.bg = CONSOLE_BG;
    g_root_scr.fb_mem = fbdev.mem;
    g_root_scr.nrows = fbdev.height / FONT_HEIGHT;
    g_root_scr.ncols = fbdev.width / FONT_WIDTH;
    g_root_scr.fbdev = fbdev;
    g_root_scr.atproc = NULL;
    g_root_scr.atproc_lock = NULL;
    memset(&g_root_scr.lock, 0, sizeof(g_root_scr.lock));
    cons_init_bufs(&g_root_scr);
    SHOW_CURSOR(&g_root_scr);
}

/*
 * Expose the console to /dev/console
 */
void
cons_expose(void)
{
    static int once = 0;
    struct ctlfs_dev ctl;
    char devname[] = "console";
    devmajor_t major;
    dev_t dev;

    /* Only run once */
    if (once) {
        return;
    }

    /* Init the attached proc mutex lock */
    g_root_scr.atproc_lock = mutex_new("console0");

    /* Register the device here */
    major = dev_alloc_major();
    dev = dev_alloc(major);
    dev_register(major, dev, &cons_cdevsw);
    devfs_create_entry(devname, major, dev, 0444);

    /* Register feat ctl */
    ctl.mode = 0666;
    ctlfs_create_node(devname, &ctl);
    ctl.devname = devname;
    ctl.ops = &cons_feat_ctl;
    ctlfs_create_entry("feat", &ctl);

    /* Register attr ctl */
    ctl.mode = 0666;
    ctlfs_create_node(devname, &ctl);
    ctl.devname = devname;
    ctl.ops = &cons_attr_ctl;
    ctlfs_create_entry("attr", &ctl);
    once ^= 1;
}

static struct cdevsw cons_cdevsw = {
    .read = dev_read,
    .write = dev_write
};

static struct ctlops cons_feat_ctl = {
    .read = ctl_feat_read,
    .write = ctl_feat_write
};

static struct ctlops cons_attr_ctl = {
    .read = ctl_attr_read,
    .write = ctl_attr_write
};

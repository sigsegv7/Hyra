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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fbdev.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

#define IS_ASCII(C) ((C) > 0 && (C) < 127)

#define PLAYER_BG 0x808080
#define MOUSE_BG  0x404040
#define GAME_BG 0x000000

#define SPRITE_WIDTH 20
#define SPRITE_HEIGHT 20
#define MAX_MOUSE_SPEED 2
#define MIN_MOUSE_SPEED 1
#define PLAYER_SPEED 30

#define SCR_WIDTH (fbattr.width)
#define SCR_HEIGHT (fbattr.height)
#define MAX_X (SCR_WIDTH - SPRITE_WIDTH)
#define MAX_Y (SCR_HEIGHT - SPRITE_HEIGHT)

/* Hit beep stuff */
#define HIT_BEEP_MSEC 50
#define HIT_BEEP_FREQ 600

static struct fbattr fbattr;
static uint32_t *framep;
static int beep_fd;
static size_t hit_count = 0;

struct player {
    int32_t x;
    int32_t y;
};

struct mouse {
    int32_t x;
    int32_t y;
    uint8_t x_inc : 1;
    uint8_t y_inc : 1;
    uint8_t speed;
};

static inline size_t
pixel_index(uint32_t x, uint32_t y)
{
    return x + y * (fbattr.pitch / 4);
}

static void
draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb)
{
    for (uint32_t xpos = x; xpos < x + width; ++xpos) {
        for (uint32_t ypos = y; ypos < y + height; ++ypos) {
            framep[pixel_index(xpos, ypos)] = rgb;
        }
    }
}

static void
update_mouse(struct mouse *mouse)
{
    draw_rect(mouse->x, mouse->y, SPRITE_WIDTH, SPRITE_HEIGHT, GAME_BG);

    /* Move the mouse in the x direction */
    if (mouse->x_inc) {
        mouse->x += mouse->speed;
    } else {
        mouse->x -= mouse->speed;
    }

    /* Move the mouse in the y direction */
    if (mouse->y_inc) {
        mouse->y += mouse->speed;
    } else {
        mouse->y -= mouse->speed;
    }

    if (mouse->x >= MAX_X) {
        mouse->x = MAX_X;
        mouse->x_inc = 0;
    } else if (mouse->x <= 0) {
        mouse->x = 0;
        mouse->x_inc = 1;
    }

    if (mouse->y >= MAX_Y) {
        mouse->y = MAX_Y;
        mouse->y_inc = 0;
    } else if (mouse->y <= 0) {
        mouse->y = 0;
        mouse->y_inc = 1;
    }

    draw_rect(mouse->x, mouse->y, SPRITE_WIDTH, SPRITE_HEIGHT, MOUSE_BG);
}

static void
beep(uint16_t msec, uint16_t freq)
{
    uint32_t payload;

    /* Can't beep :(  */
    if (beep_fd < 0) {
        return;
    }

    payload = freq;
    payload |= (msec << 16);
    write(beep_fd, &payload, sizeof(payload));
}

static void
score_increment(struct player *p, struct mouse *m)
{
    printf("\033[31;40mSCORE: %d\033[0m\n", ++hit_count);

    if (m->speed < MAX_MOUSE_SPEED) {
        m->speed += 1;
    } else {
        m->speed = MIN_MOUSE_SPEED;
    }
}

static bool
mouse_collide(struct player *p, struct mouse *m)
{
    bool detected = false;
    bool x_overlap, y_overlap;

    x_overlap = p->x < (m->x + SPRITE_WIDTH) &&
        (p->x + SPRITE_WIDTH) > m->x;
    y_overlap = p->y < (m->y + SPRITE_HEIGHT) &&
        (p->y + SPRITE_HEIGHT) > m->y;
    detected = x_overlap && y_overlap;

    /*
     * Play a little ACK sound and reset the game
     * if we collide
     */
    if (detected) {
        beep(HIT_BEEP_MSEC, HIT_BEEP_FREQ);

        /* Clear the sprites */
        draw_rect(m->x, m->y, SPRITE_WIDTH, SPRITE_HEIGHT, GAME_BG);
        draw_rect(p->x, p->y, SPRITE_WIDTH, SPRITE_HEIGHT, GAME_BG);

        m->x = 0;
        m->y = 0;
        m->x_inc ^= 1;
        m->y_inc ^= 1;
        score_increment(p, m);
    }

    return detected;

}

static void
game_loop(void)
{
    struct timespec ts;
    struct mouse mouse;
    struct player p;
    char c;
    bool running = true;

    ts.tv_sec = 0;
    ts.tv_nsec = 7000000;

    /* Setup the player */
    p.x = 0;
    p.y = 0;

    /* Setup the mouse */
    mouse.x = MAX_X;
    mouse.y = MAX_Y;
    mouse.x_inc = 0;
    mouse.y_inc = 0;
    mouse.speed = MIN_MOUSE_SPEED;

    /* Draw player and mouse */
    draw_rect(p.x, p.y, SPRITE_WIDTH, SPRITE_HEIGHT, PLAYER_BG);
    draw_rect(mouse.x, mouse.y, SPRITE_WIDTH, SPRITE_HEIGHT, MOUSE_BG);

    while (running) {
        if (mouse_collide(&p, &mouse)) {
            continue;
        }

        c = getchar();
        sleep(&ts, &ts);
        update_mouse(&mouse);

        if (IS_ASCII(c)) {
            draw_rect(p.x, p.y, SPRITE_WIDTH, SPRITE_HEIGHT, GAME_BG);
        }

        switch (c) {
        case 'w':
            p.y -= PLAYER_SPEED;
            if (p.y <= 0) {
                p.y = 0;
            }
            break;
        case 'a':
            p.x -= PLAYER_SPEED;
            if (p.x <= 0) {
                p.x = 0;
            }
            break;
        case 's':
            p.y += PLAYER_SPEED;
            if (p.y > MAX_Y){
                p.y = MAX_Y;
            }
            break;
        case 'd':
            p.x += PLAYER_SPEED;
            if (p.x > MAX_X) {
                p.x = MAX_X;
            }
            break;
        case 'q':
            running = false;
        default:
            continue;
        }

        draw_rect(p.x, p.y, SPRITE_WIDTH, SPRITE_HEIGHT, PLAYER_BG);
    }
}

int
main(void)
{
    int fb_fd, fbattr_fd, prot;
    size_t fb_size;
    char c;

    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) {
        return fb_fd;
    }

    fbattr_fd = open("/ctl/fb0/attr", O_RDONLY);
    if (fbattr_fd < 0) {
        close(fb_fd);
        return fbattr_fd;
    }

    beep_fd = open("/dev/beep", O_WRONLY);
    read(fbattr_fd, &fbattr, sizeof(fbattr));
    close(fbattr_fd);

    fb_size = fbattr.height * fbattr.pitch;
    prot = PROT_READ | PROT_WRITE;
    framep = mmap(NULL, fb_size, prot, MAP_SHARED, fb_fd, 0);

    game_loop();
    printf("\033[35;40mYOUR FINAL SCORE: %d\033[0m\n", hit_count);

    /* Cleanup */
    close(beep_fd);
    munmap(framep, fb_size);
    close(fb_fd);
    return 0;
}

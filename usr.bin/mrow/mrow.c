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
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libgfx/gfx.h>
#include <libgfx/draw.h>

#define IS_ASCII(C) ((C) > 0 && (C) < 127)

#define PLAYER_BG 0x808080
#define MOUSE_BG  0x404040
#define GAME_BG 0x000000

#define SPRITE_WIDTH 20
#define SPRITE_HEIGHT 20
#define MAX_MOUSE_SPEED 2
#define MIN_MOUSE_SPEED 1
#define PLAYER_SPEED 30

#define SCR_WIDTH (gfx_ctx.fbdev.width)
#define SCR_HEIGHT (gfx_ctx.fbdev.height)
#define MAX_X (SCR_WIDTH - SPRITE_WIDTH)
#define MAX_Y (SCR_HEIGHT - SPRITE_HEIGHT)

/* Hit beep stuff */
#define HIT_BEEP_MSEC 50
#define HIT_BEEP_FREQ 600

static struct gfx_ctx gfx_ctx;
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

static void
draw_sprite(uint32_t x, uint32_t y, uint32_t color)
{
    struct gfx_shape sprite_shape = GFX_SHAPE_DEFAULT;

    sprite_shape.x = x;
    sprite_shape.y = y;
    sprite_shape.width = SPRITE_WIDTH;
    sprite_shape.height = SPRITE_HEIGHT;
    sprite_shape.color = color;
    gfx_draw_shape(&gfx_ctx, &sprite_shape);
}

static void
update_mouse(struct mouse *mouse)
{
    draw_sprite(mouse->x, mouse->y, GAME_BG);

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

    draw_sprite(mouse->x, mouse->y, MOUSE_BG);
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
        draw_sprite(m->x, m->y, GAME_BG);
        draw_sprite(p->x, p->y, GAME_BG);

        m->x = 0;
        m->y = rand() % MAX_Y;
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
    draw_sprite(p.x, p.y, PLAYER_BG);
    draw_sprite(mouse.x, mouse.y, MOUSE_BG);

    while (running) {
        if (mouse_collide(&p, &mouse)) {
            continue;
        }

        c = getchar();
        sleep(&ts, &ts);
        update_mouse(&mouse);

        if (IS_ASCII(c)) {
            draw_sprite(p.x, p.y, GAME_BG);
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

        draw_sprite(p.x, p.y, PLAYER_BG);
    }
}

int
main(void)
{
    int error;
    char c;

    error = gfx_init(&gfx_ctx);
    if (error < 0) {
        printf("failed to init libgfx\n");
        return error;
    }

    beep_fd = open("/dev/beep", O_WRONLY);
    game_loop();
    printf("\033[35;40mYOUR FINAL SCORE: %d\033[0m\n", hit_count);

    /* Cleanup */
    close(beep_fd);
    gfx_cleanup(&gfx_ctx);
    return 0;
}

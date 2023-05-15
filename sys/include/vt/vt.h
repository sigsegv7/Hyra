/*
 * Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
 * 3. Neither the name of VegaOS nor the names of its
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

#ifndef _VT_H_
#define _VT_H_

#include <sys/types.h>
#include <sync/spinlock.h>

typedef enum {
    CURSOR_NONE,
    CURSOR_BLOCK,
} vt_cursor_type_t;

/*
 * Describes the attributes
 * of the virtual terminal.
 */

struct vt_attr {
    uint32_t bg;
    uint32_t text_bg;
    uint32_t text_fg;
    uint32_t cursor_bg;
    vt_cursor_type_t cursor_type;
};

/*
 * Describes the state of
 * the virtual terminal.
 */

struct vt_state {
    uint32_t cursor_x;
    uint32_t cursor_y;
};

/*
 * Describes the virtual
 * terminal itself.
 */

struct vt_descriptor {
    uint32_t *fb_base;
    struct vt_attr attr;
    struct vt_state state;
    struct spinlock lock;
};

void vt_write(struct vt_descriptor *vt, const char *str, size_t len);
void vt_reset(struct vt_descriptor *vt);

int vt_init(struct vt_descriptor *vt, const struct vt_attr *attr,
            uint32_t *fb_base);

int vt_chattr(struct vt_descriptor *vt, const struct vt_attr *attr);

#endif

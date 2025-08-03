# The OSMORA Display Architecture (ODA)

Written by Ian M. Moffett

## Introduction

The OSMORA Display Architecture (ODA) is a protocol describing how
a compositor should create and manage graphical windows. A graphical
session in Hyra consists of a compositor, window management system and
management of user input.

There are many existing display architectures out there. Take for instace, the X11
protocol. X11 for example, has the concept of an "X server" in which window managers
or certain graphical programs would connect to as a means of performing interprocess
communication (IPC). The idea is that X will service graphics related requests from
the window manager or compositor.

While this works just fine, the highly centralized nature of X11 or similar protocols
may complicate the flexibility of the graphics stack. On the other hand with ODA, a
compositor links with the ODA library and becomes the server for window managers running
on the system. The idea of ODA is to minimize complexity while preserving flexibility.

Additionally, the compositor should provide its own API for use by window management
software.

## Scope

This document serves to describe common OSMORA Display Architecture (ODA) concepts
as well as providing basic example C sources showcasing how compositors and window
managers may interface with the described APIs for various needs.

## Terminology

### OSMORA Display Architecture (ODA):

OSMORA protocol defining common interfaces for C2W and
W2C interactions.

### Compositor to window (C2W):

Describes the direction of communication originating from
a compositor and directed towards a specific window:

``COMPOSITOR -> WINDOW``

### Window to compositor (W2C):

Describes the direction of communication originating from
a specific window to a compositor running on the system:

``WINDOW -> COMPOSITOR``

## Architecture

```
+-------------+
| LIBGFX      |
+-------------+
       ^
       |         linked with libgfx
       V
+-------------+
| COMPOSITOR  |
+-------------+ <---+  signal
  |    |    |       |
  |    |    |       |   c2w: <winop: e.g., close>
  |    |    |       |   w2c: <winop to accept: e.g., close>
 WIN  WIN  WIN  <---+
```

### C2W signal flow:

```
-- CLOSE SIGNAL EXAMPLE --

WINDOW RECEIVES CLOSE SIGNAL
           |
 ECHO BACK TO COMPOSITOR
           |
    YES ---+--- NO
    |           |
    |           V
    |           nothing happens
    |
    V
    window is destroyed by compositor
```

## Libgfx

The Hyra userspace includes ``libgfx`` which is a low-level graphics library aimed
to facilitate drawing on the screen and performing various graphical operations while
decoupling from the concept of compositors, windows and the ODA as a whole. In other words,
libgfx has no knowledge of anything outside of the framebuffer and itself.

The following is an example of how one may draw a yellow square at
x/y (30,0):

```c
#include <libgfx/gfx.h>     /* Common routines/defs */
#include <libgfx/draw.h>    /* Drawing related routines/defs */

int
main(void)
{
    struct gfx_ctx ctx;
    struct gfx_shape sh = GFX_SHAPE_DEFAULT;
    int error;

    /* Set the x/y and color */
    sh.x = 30;
    sh.y = 0;
    sh.color = GFX_YELLOW

    error = gfx_init(&ctx);
    if (error < 0) {
        printf("gfx_init returned %d\n", error);
        return error;
    }

    /* Draw the square and cleanup */
    gfx_draw_shape(&ctx, &sh);
    gfx_cleanup(&ctx);
    return 0;
}
```

## Liboda

The Hyra userspace includes the ``liboda`` library which includes various
interfaces conforming to the OSMORA Display Architecture (ODA).

### Linking a compositor with liboda

In order for an ODA compliant compositor to reference library
symbols for ``liboda``, it should use the following linker flags:

``... -loda -logfx``

### ODA Session

For the ODA library to keep track of state, it relies on an ``oda_state``
structure defined in ``liboda/oda.h``. Additionally, in order for any ODA
library calls to be made, the compositor must initialize the library with
``oda_init()`` like in the following example:

```
#include <liboda/oda.h>

struct oda_state state;
int error;

/* Returns 0 on success */
error = oda_init(&state);
...
```

Upon failure of ``oda_init()`` a negative POSIX errno value
is returned (see ``sys/errno.h``).

### Using liboda to request windows

A compositor may request windows from the ODA by using
``oda_reqwin()`` like in the following example:

```
#include <liboda/oda.h>

...
struct oda_wattr wattr;
struct oda_state state;

...

wattr.session = &state;
wattr.parent = NULL;
wattr.bg = GFX_YELLOW;
wattr.x = 200;
wattr.y = 150;
wattr.w = 120;
wattr.h = 300;

/* Returns 0 on success */
error = oda_reqwin(&wattr, &win);
```

Arguments passed to ``oda_reqwin()`` are first stored in a ``struct oda_wattr``
structure to minimize the number of parameters used in the function signature.

Upon failure of ``oda_reqwin()`` a negative POSIX errno value
is returned (see ``sys/errno.h``).

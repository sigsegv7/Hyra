# The Hyra control filesystem (ctlfs)

Written by Ian M. Moffett

## Rationale

Historically, Operating Systems of the Unix family typically relied
on syscalls like ``ioctl()`` or similar to perform operations (e.g., making calls through a driver)
via some file descriptor. Let's say for example, one wanted to acquire the framebuffer
dimensions of a given framebuffer device. To start, they'd acquire a file descriptor
by calling ``open()`` or similar on it. Then they'd make their ``ioctl()`` call.

```c
int fd = ...;

ioctl(fd, IOCTL_FBINFO, &fb_info);
...
```

While this works fine and is relatively simple to use from the user's
perspective, it is very clunky when you pop the hood and peer into the
inner-workings of it within the kernel. The number of possible requests
that can be passed through a file descriptor can grow quite rapidly which
can require really large switch statements within the drivers that implement
an ``ioctl()`` interface.

## Replacing ``ioctl()``

Hyra provides ctlfs, an abstract in-memory filesystem designed for
setting/getting various kernel / driver parameters and state via
the filesystem API. The control filesystem consists of several
instances of two fundamentals: "control nodes" and "control entries".

### Control nodes

Control nodes are simply directories within the ``/ctl`` root. For example,
console specific control files are in the ``/ctl/console`` node.

### Control entries

Control entries are simply files within specific control nodes. For example
console features may be find in the ``consfeat`` entry of the ``console`` node
(i.e., ``/ctl/console/consfeat``).

See ``sys/include/sys/console.h`` and ``sys/fs/ctlfs.h`` for more
information.

## The ctlfs API

The Hyra kernel provides an API for subsystems and drivers
to create their own ctlfs entries and nodes. This may be found
in sys/include/fs/ctlfs.h

### Control operations

Each control entry must define their own set of
"control operations" described by the ``ctlops`` structure:

```c
struct ctlops {
    int(*read)(struct ctlfs_dev *cdp, struct sio_txn *sio);
    int(*write)(struct ctlfs_dev *cdp, struct sio_txn *sio);
    ...
};
```

NOTE: Callbacks defined as ``NULL`` will be
ignored and unused.

## "Meow World": Creating a ctlfs entry

```c
#include <sys/types.h>
#include <sys/sio.h>
#include <fs/ctlfs.h>

static const struct ctlops meow_ctl;

/*
 * Ctlfs read callback - this will be called
 * when "/ctl/meow/hii" is read.
 */
static int
ctl_meow_read(struct ctlfs_dev *cdp, struct sio_txn *sio)
{
    char data[] = "Meow World!""

    /* Clamp the input length */
    if (sio->len > sizeof(data)) {
        sio->len = sizeof(data)
    }

    /* End of the data? */
    if ((sio->offset + sio->len) > sizeof(data)) {
        return 0;
    }

    /* Copy the data and return the length */
    memcpy(sio->buf, &data[sio->offset], sio->len);
    return sio->len;
}

static int
foo_init(void)
{
    char ctlname[] = "meow";
    struct ctlfs_dev ctl;

    /*
     * Here we create the "/ctl/meow" node.
     */
    ctl.mode = 0444;
    ctl.devname = devname;
    ctlfs_create_node(devname, &ctl);

    ctl.ops = &fb_size_ctl;
    ctlfs_create_entry("attr", &ctl);
    return 0;
}

static const struct ctlops meow_ctl = {
    .read = ctl_meow_read,
    .write = NULL,
};
```

The Hyra Operating System
=========================

Welcome to the Hyra Operating System project! Hyra is an experimental
operating system inspired by BSD and Plan 9 while being entirely written from scratch.
Hyra aims to rethink core fundamentals in modern operating system design in order to
create new and improved architectural ideas.


Project Goal:
--------------
The goal of this project is to redefine what modern operating systems are while taking inspiration from BSD. Hyra does
not use CPIO for its initramfs like other operating systems typically would and instead uses the [OSMORA Archive Format (OMAR)](https://osmora.org/oap/oap-0005).

What Hyra is NOT:
--------------
Hyra is *NOT* Linux, nor does extend or share any sources with any existing
operating systems as it is written entirely from scratch. Hyra is *NOT* intended as a "toy" project as it is aimed to be the used as the main operating system for internal OSMORA operations and infrastructure.

Getting Started:
----------------
To build Hyra you'll need to bootstrap the project which is essentially just fetching dependencies for the project. This can be done by running the bootstrap script within the project root: `./bootstrap`.

Next, to configure for x86_64 just run configure:

`./configure`

After running the configure script, you can now actually build Hyra:

`./hyra-build.sh`

This will generate a new `Hyra.iso` file.


Default User:
----------------
Upon booting, the `login` program will ask for user credentials. The default username is `root` and the default
password is also `root`.

Programs:
----------------
The Hyra userspace provides the user various programs that they can run. Examples of
such programs include:

- ``beep`` - Play a tone
- ``cat`` - Print files to stdout
- ``date`` - Get the current date or set system time
- ``echo`` - Print a line of text
- ``elfdump`` - Get information about an ELF binary
- ``fetch`` - System fetch! A must have :~)
- ``getconf`` - Get system configuration values
- ``mex`` - OSMORA hexdump utility
- ``sleep`` - Sleep for a number of seconds
- ``kmsg`` - Read the kernel message buffer
- ``readcore`` - Read coredump files
- ``oasm`` - OSMORA [OSMX64](https://github.com/sigsegv7/OSMX64) Assembler
- ``oemu`` - OSMORA [OSMX64](https://github.com/sigsegv7/OSMX64) Emulator
- ``kstat`` - Read kernel statistics
- ``dmidump`` - Dump DMI/SMBios information
- ``screensave`` - Glitch art screensaver
- ``whoami`` - Print effective user name
- ``sysctl`` - Runtime kernel parameters
- ``notes`` - Music box

And more! See ``usr.bin/*``

Libraries:
----------------
The Hyra userspace additionally provides the user various libraries that they can
link with. Examples of such libraries include:

- ``libc`` - C library (link flag: ``-lc``)
- ``libgfx`` - Low-level graphics (link flag: ``-lgfx``)

And more! See ``lib/*``

Documentation:
--------------
Documentation will be in the form of comments throughout the codebase and can also be found in:

- ``share/man/*``: Man pages
- ``share/contrib``: Information on contributing
- ``share/docs/kernel``: Kernel documentation
- ``share/docs/lib``: Library documentation

# Maintainers (by author)
--------------
| Maintainer         | Component          |
|--------------------|--------------------|
| <ian@osmora.org>   | Hyra AMD64 Kernel  |
| <ian@osmora.org>   | User C Library     |
| <ian@osmora.org>   | NVMe Driver        |
| <ian@osmora.org>   | AHCI Driver        |
| <ian@osmora.org>   | xHCI Driver        |
| <ian@osmora.org>   | RTL8139 Driver     |
| <ian@osmora.org>   | E1000E Driver      |
| <ian@osmora.org>   | ET131X Driver      |
| <ian@osmora.org>   | PCI Driver         |
| <ian@osmora.org>   | PCIe Driver        |
| <quinn@osmora.org> | PCI Driver         |
| <quinn@osmora.org> | User C Library     |
| <quinn@osmora.org> | Killing MS         |

--------------
# To-do

```
[ ] kern: dev: AHCI DCDR cache (<ian@osmora.org>)
[ ] kern: Worker threads (<ian@osmora.org>)
[ ] kern: Multithreaded driver startup (<quinn@osmora.org>)
[ ] libc: Slab allocator (<quinn@osmora.org>)
...
```

Hyra running on bare metal:
--------------
![Hyra](./.github/assets/hyra.png)

License:
--------
This project is licensed under the BSD-3 clause (SPDX Identifier: BSD-3-Clause)

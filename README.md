The Hyra Operating System
=========================

Welcome to the Hyra Operating System project!

Project Goal:
--------------
The goal of this project is to redefine what modern operating systems are while taking inspiration from BSD. Hyra does not use
POSIX by default and instead uses the [OSMORA Uniform System Interface (OUSI)](https://osmora.org/oap/oap-0002). Hyra also does
not use CPIO for its initramfs like other operating systems typically would and instead uses the [OSMORA Archive Format (OMAR)](https://osmora.org/oap/oap-0005).

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
The Hyra userspace provides the user various programs that they can run, examples of
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

And more! See ``usr.bin/*``

Documentation:
--------------
Documentation will be in the form of comments throughout the codebase and can also be found in the share/ directory within the project root.

Hyra running on bare metal:
--------------
![Hyra](./.github/assets/hyra.png)

License:
--------
This project is licensed under the BSD-3 clause (SPDX Identifier: BSD-3-Clause)

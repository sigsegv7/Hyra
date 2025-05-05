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

Now you'll need to build the cross compiler by running:

`make cross`

This may take awhile so just sit back, relax and do something else like... well I'm not you so
I don't know what you like.

After the cross compiler is done building you can build and run the project in a virtual machine:

`make; make run`

Documentation:
--------------
Documentation will be in the form of comments throughout the codebase and can also be found in the share/ directory within the project root.

License:
--------
This project is licensed under the BSD-3 clause (SPDX Identifier: BSD-3-Clause)

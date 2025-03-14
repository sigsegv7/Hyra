#!/bin/bash

#
# Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of Hyra nor the names of its contributors may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

################################################################
# Toolchain configuration
################################################################

set -e

if [ "$1" == "amd64" ]
then
    TARGET="x86_64-hyra"
else
    TARGET="$1-elf"
fi

BINUTILS_VERSION=2.42
GCC_VERSION=13.2.0
BINUTILS_NAME="binutils-$BINUTILS_VERSION"
BINUTILS_TARBALL="$BINUTILS_NAME.tar.xz"
BINUTILS_URL="https://ftp.gnu.org/gnu/binutils/$BINUTILS_TARBALL"
GCC_NAME="gcc-$GCC_VERSION"
GCC_TARBALL="$GCC_NAME.tar.xz"
GCC_URL="https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/$GCC_TARBALL"

# Remove debugging information, it takes up a lot of space
export CFLAGS="-g0"
export CXXFLAGS="-g0"

# System-dependent configuration
SYSTEM_NAME="$(uname -s)"
MAKE="make"
NPROC="nproc"
if [ "$SYSTEM_NAME" = "OpenBSD" ]; then
        MAKE="gmake"
        NPROC="sysctl -n hw.ncpuonline"
elif [ "$SYSTEM_NAME" = "FreeBSD" ]; then
        MAKE="gmake"
        NPROC="sysctl -n hw.ncpu"
elif [ "$SYSTEM_NAME" = "Darwin" ]; then
        NPROC="sysctl -n hw.ncpu"
fi
CORES="$($NPROC)"

# Set build paths
PREFIX="$(pwd)/cross"
export PATH="$PREFIX/bin:$PATH"

# Create build directory
mkdir -p "$PREFIX/build"
cd "$PREFIX/build"

################################################################
# Download and extract sources
################################################################

if [ ! -f $BINUTILS_TARBALL ]; then
	echo "Downloading binutils..."
	curl $BINUTILS_URL -o $BINUTILS_TARBALL
fi
if [ ! -f $GCC_TARBALL ]; then
	echo "Downloading gcc..."
	curl $GCC_URL -o $GCC_TARBALL
fi

echo "Extracting binutils..."
rm -rf $BINUTILS_NAME
tar -xf $BINUTILS_TARBALL
echo "Extracting gcc..."
rm -rf $GCC_NAME
tar -xf $GCC_TARBALL

################################################################
# Build packages
################################################################

echo "Removing previous builds..."
rm -rf build-gcc build-binutils

# Binutils build
mkdir build-binutils

echo "Applying binutils patch"
cp -r $BINUTILS_NAME $BINUTILS_NAME-copy
patch -s -p0 < ../../builddeps/binutils.patch
rm -rf $BINUTILS_NAME-copy

echo "Configuring $BINUTILS_NAME..."
cd build-binutils
../$BINUTILS_NAME/configure --target="$TARGET" --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror --enable-targets=all
echo "Building $BINUTILS_NAME..."
$MAKE -j$CORES
echo "Installing $BINUTILS_NAME..."
$MAKE install
echo "Cleaning $BINUTILS_NAME..."
cd ..
rm -rf $BINUTILS_NAME build-binutils

# GCC build
echo "Downloading prerequisites for $GCC_NAME..."
cd $GCC_NAME
contrib/download_prerequisites
cd ..

echo "Applying GCC patch"
cp -r $GCC_NAME $GCC_NAME-copy
patch -s -p0 < ../../builddeps/gcc.patch
rm -rf $GCC_NAME-copy

echo "Configuring $GCC_NAME..."
mkdir build-gcc
cd build-gcc
../$GCC_NAME/configure --target="$TARGET" --with-sysroot=/ --prefix="$PREFIX" --disable-nls --enable-languages=c,c++,lto --disable-multilib
echo "Building all-gcc..."
$MAKE all-gcc -j"$CORES"
echo "Building all-target-libgcc..."
$MAKE all-target-libgcc -j"$CORES"
echo "Installing $GCC_NAME..."
$MAKE install-gcc
echo "Installing target-libgcc..."
$MAKE install-target-libgcc
echo "Cleaning $GCC_NAME..."
cd ..
rm -rf $GCC_NAME build-gcc

################################################################
# Cleanup
################################################################

"$TARGET"-ld -v
"$TARGET"-gcc --version | head -n1
echo "Build complete, binaries are in $PREFIX"
echo "Finished in $(($SECONDS / 60)) minutes and $(($SECONDS % 60)) seconds"

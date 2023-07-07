#!/bin/bash

#
# Copyright (c) 2023 Ian Marco Moffett and the VegaOS team.
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
# 3. Neither the name of VegaOS nor the names of its contributors may be used
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
# Configuration
################################################################

# Exit if there is an error
set -e

# Target to use
TARGET=$1-elf

# Versions to build
# Always use the latest working version (test before updating)
BUT_VER=2.40
GCC_VER=13.1.0

# Tar file extension to use
# Always use the one with the smallest file size (check when updating version)
BUT_EXT=xz
GCC_EXT=xz

# Multicore builds

if [ "$(uname)" = "FreeBSD" ]; then
  CORES=$(($(sysctl -n hw.ncpu) + 1))
  LOAD=$(sysctl -n hw.ncpu)
else
  CORES=$(($(nproc) + 1))
  LOAD=$(nproc)
fi

PREFIX="$(pwd)/cross"
export PATH="$PREFIX/bin:$PATH"

echo "Building $TARGET Binutils $BUT_VER and GCC $GCC_VER..."
echo "Cores: $CORES, load: $LOAD"

################################################################
# Source Tarballs
################################################################

BUT_TARBALL=binutils-$BUT_VER.tar.$BUT_EXT
GCC_TARBALL=gcc-$GCC_VER.tar.$GCC_EXT

mkdir -p buildcc
cd buildcc

# Download tarballs
echo "Downloading Binutils tarball..."
if [ ! -f $BUT_TARBALL ]; then
    wget https://ftp.gnu.org/gnu/binutils/$BUT_TARBALL
fi

echo "Downloading GCC tarball..."
if [ ! -f $GCC_TARBALL ]; then
    wget https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/$GCC_TARBALL
fi

# Unzip tarballs
printf "%s" "Unzipping Binutils tarball"
tar -xf $BUT_TARBALL
echo "" # Newline :~)
printf "%s" "Unzipping GCC tarball"
tar -xf $GCC_TARBALL

################################################################
# Building
################################################################

echo "Removing old build directories..."
rm -rf buildcc-gcc build-binutils

# Build binutils
mkdir buildcc-binutils
cd buildcc-binutils
echo "Configuring Binutils..."
../binutils-$BUT_VER/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
echo "Building Binutils..."
make -j$CORES -l$LOAD
echo "Installing Binutils..."
make install -j$CORES -l$LOAD
cd ..

# Build gcc
cd gcc-$GCC_VER
echo "Downloading prerequisites for GCC..."
contrib/download_prerequisites
cd ..
mkdir buildcc-gcc
cd buildcc-gcc
echo "Configuring GCC..."
../gcc-$GCC_VER/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers
echo "Building all-gcc..."
make all-gcc -j$CORES -l$LOAD
echo "Building all-target-libgcc..."
make all-target-libgcc -j$CORES -l$LOAD
echo "Installing GCC..."
make install-gcc -j$CORES -l$LOAD
echo "Installing target-libgcc..."
make install-target-libgcc -j$CORES -l$LOAD
cd ../..

echo "Removing build directory..."
rm -rf buildcc

echo "Build complete, binaries are in $PREFIX/bin"

################################################################
# Basic Testing (just prints info for now)
################################################################

echo "Testing GCC..."
$TARGET-gcc -v

echo "Testing LD..."
$TARGET-ld -v

echo "Done!"

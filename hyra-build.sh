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

set -e

RAMFS_TOOL="tools/omar/bin/omar"
RAMFS_NAME="ramfs.omar"
install_flag="false"
NTHREADS="-j$(nproc)"

###############################
# Generate sysroot skeleton
###############################
sysroot_skel() {
    mkdir -p base/usr/lib/
    mkdir -p base/usr/sbin/
    mkdir -p base/usr/bin/
    mkdir -p base/boot/
    mkdir -p base/usr/include/sys/
    mkdir -p base/usr/rc

    cp -r rc/* base/usr/rc
    cp -f sys/include/sys/*.h base/usr/include/sys
    cp -r etc base/etc/

    # Populate ESP
    make stand/boot/
    cp stand/boot/*.EFI iso_root/EFI/BOOT/
}

iso_root_skel() {
    mkdir -p iso_root/boot/
    mkdir -p iso_root/EFI/BOOT/
}

###############################
# Generate ISO root
###############################
gen_iso_root() {
    cp $RAMFS_NAME iso_root/boot/
    cp builddeps/limine.conf stand/limine/limine-bios.sys \
        stand/limine/limine-bios-cd.bin stand/limine/limine-uefi-cd.bin iso_root/
    cp builddeps/wallpaper.jpg iso_root/boot/
}

##################################
# Stage 1 - generate isofs
#
# ++ ARGS ++
# $1: ISO output name
# --      --
##################################
gen_isofs() {
    cp base/boot/* iso_root/boot/
    xorriso -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4\
        -boot-info-table --efi-boot limine-uefi-cd.bin -efi-boot-part \
        --efi-boot-image --protective-msdos-label iso_root -o $1 > /dev/null
    stand/limine/limine bios-install $1
}

build() {
    echo "-- Building libs --"
    make libs

    echo "-- Building world --"
    make $NTHREADS sbin bin

    echo "-- Building kernel --"
    make $NTHREADS base/boot/hyra.krq
}

####################################
# Stage 1 - build production media
####################################
stage1() {
    iso_root_skel
    sysroot_skel

    echo "[*] stage1: Build kernel"
    build

    echo "[*] stage1: Generate stage 1 RAMFS via OMAR"
    $RAMFS_TOOL -i base/ -o $RAMFS_NAME

    echo "[*] stage1: Generate stage 1 ISOFS (production)"
    gen_iso_root
    gen_isofs "Hyra.iso"

    # Clean up
    rm $RAMFS_NAME
    rm -r iso_root
}

#################################
# Stage 2 - build install media
#################################
stage2() {
    make clean
    rm -f base/boot/hyra.krq

    iso_root_skel
    sysroot_skel

    echo "[*] stage2: Generate stage 2 RAMFS via OMAR"
    mv Hyra.iso base/boot/
    $RAMFS_TOOL -i base/ -o $RAMFS_NAME

    echo "[*] stage2: Build kernel"
    gen_iso_root
    export KBUILD_ARGS="-D_INSTALL_MEDIA=1"
    build

    echo "[*] stage2: Generate stage 2 ISOFS (installer)"
    gen_isofs "Hyra-install.iso"

    # Clean up
    rm $RAMFS_NAME
    rm base/boot/Hyra.iso
    rm -r iso_root
}

##################################
# Clean up completly after build
##################################
hard_clean() {
    make clean
    rm -rf base/
}

##################################
# Build results
#
# ++ ARGS ++
# $1: ISO output name
# --      --
##################################
result() {
    echo "-------------------------------------------"
    echo "Build finish"

    if [[ $1 == "Hyra-install.iso" ]]
    then
        hard_clean # XXX: For safety
        echo "Installer is at ./Hyra-install.iso"
        echo "!!NOTE!!: OSMORA is not responsible for incidental data loss"
    else
        echo "Boot image is at ./Hyra.iso"
    fi

    echo "Finished in $(($SECONDS / 60)) minutes and $(($SECONDS % 60)) seconds"
    echo "-------------------------------------------"
}

while getopts "ih" flag
do
    case "${flag}" in
        i) install_flag="true"
            ;;
        *)
            echo "Hyra build script"
            echo "[-i] Build installer"
            echo "[-h] Help"
            exit 1
            ;;
    esac
done

if [[ ! -f ./configure ]]
then
    echo "[!] Please bootstrap and configure Hyra!"
    echo "[!] Error in stage 1, exiting"
    exit 1
fi

if [[ ! -f Makefile ]]
then
    echo "[!] 'Makefile' not found, did you run './configure'?"
    echo "[!] Error in stage 1, exiting"
fi

echo "-- Begin stage 1 --"
stage1

if [[ $install_flag != "true" ]]
then
    echo "[?] Not building installer (-i unset)"
    echo "-- Skipping stage 2 --"
    result "Hyra.iso"
else
    echo "-- Begin stage 2 --"
    stage2
    result "Hyra-install.iso"
fi

if [[ $install_flag == "true" ]]
then
    make clean
    rm -rf base/
fi

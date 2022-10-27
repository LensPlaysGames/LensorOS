#!/bin/bash

# Copyright 2022, Contributors To LensorOS.
# All rights reserved.
#
# This file is part of LensorOS.
#
# LensorOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LensorOS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LensorOS. If not, see <https://www.gnu.org/licenses/>.


#     This script is meant to be ran from linux-gnu,
#     and creates the LensorOS toolchain for w64-mingw32.

#     Dependencies:
#     |-- Curl
#     |-- GNU Diff/Patch
#     |-- GNU Make
#     |-- GNU Tar (xz-utils)
#     |-- x86_64-linux-gnu compiler
#     `-- x86_64-w64-mingw32 cross compiler

# Set variables.
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TARGET="x86_64-lensor"
PREFIX="$ScriptDirectory/wincross"
SYSROOT="$ScriptDirectory/../root"
if [ -z "$BINUTILS_VERSION" ]; then
    BINUTILS_VERSION="2.38";
fi
BINUTILS_STRING="binutils-$BINUTILS_VERSION"
if [ -z "$GCC_VERSION" ]; then
    GCC_VERSION="12.1.0"
fi
GCC_STRING="gcc-$GCC_VERSION"
# Ensure known working directory.
cd $ScriptDirectory
# Download, extract, and patch source archives if they haven't been already.
if [ ! -d $BINUTILS_STRING ]; then
    echo -e "\n\n -> Downloading GNU Binutils Source Archive\n\n"
    curl https://mirrors.kernel.org/gnu/binutils/$BINUTILS_STRING.tar.xz \
         --output $BINUTILS_STRING.tar.xz
    echo -e "\n\n -> Extracting GNU Binutils\n\n"
    mkdir -p $BINUTILS_STRING
    tar -xf $BINUTILS_STRING.tar.xz -C .
    echo -e "\n\n -> Patching GNU Binutils\n\n"
    patch -s -u -p0 < $ScriptDirectory/$BINUTILS_STRING-lensor.patch
fi
if [ ! -d $GCC_STRING ]; then
    echo -e "\n\n -> Downloading GNU Compiler Collection Source Archive\n\n"
    curl https://mirrors.kernel.org/gnu/gcc/$GCC_STRING/$GCC_STRING.tar.xz \
         --output $GCC_STRING.tar.xz
    echo -e "\n\n -> Extracting GNU Compiler Collection\n\n"
    mkdir -p $GCC_STRING
    tar -xf $GCC_STRING.tar.xz -C .
    echo -e "\n\n -> Downloading GNU Compiler Collection Prerequisites\n\n"
    cd $GCC_STRING
    ./contrib/download_prerequisites
    cd $ScriptDirectory
    echo -e "\n\n -> Patching GNU Compiler Collection\n\n"
    patch -s -u -p0 < $ScriptDirectory/$GCC_STRING-lensor.patch
fi
if [ ! -d $SYSROOT ] ; then
    echo -e "\n\n -> Bootstrapping build-system system root at $SYSROOT\n\n"
    mkdir -p $SYSROOT
    # Copy base filesystem with pre-built libc
    #     binaries into newly-created sysroot.
    # This solves the issue of having to build a
    #     bootstrap version of the compiler first.
    cp -r ../base/* $SYSROOT/
    # Copy header files from libc to sysroot.
    cd ../user/libc/
    find ./ -name '*.h' -exec cp --parents '{}' -t $SYSROOT/inc ';'
fi
# Create output build directory.
cd $ScriptDirectory
mkdir -p wincross
if [ ! -d $BINUTILS_STRING"-winbuild" ] ; then
    # Configure binutils.
    echo -e "\n\n -> Configuring GNU Binutils\n\n"
    cd $ScriptDirectory
    mkdir -p $BINUTILS_STRING-winbuild
    cd $BINUTILS_STRING-winbuild
    $ScriptDirectory/$BINUTILS_STRING/configure \
        --build x86_64-linux-gnu                \
        --host x86_64-w64-mingw32               \
        --target=$TARGET                        \
        --prefix="$PREFIX"                      \
        --with-sysroot="$SYSROOT"               \
        --disable-nls                           \
        --disable-werror
    # Build binutils.
    echo -e "\n\n -> Building GNU Binutils\n\n"
    make -j
    echo -e "\n\n -> Installing GNU Binutils\n\n"
    make install -j
    cd ..
fi
if [ ! -d $GCC_STRING"-winbuild" ] ; then
    # Configure GCC.
    echo -e "\n\n -> Configuring GNU Compiler Collection\n\n"
    cd $ScriptDirectory
    mkdir -p $GCC_STRING-winbuild
    cd $GCC_STRING-winbuild
    $ScriptDirectory/$GCC_STRING/configure \
        --build x86_64-linux-gnu           \
        --host x86_64-w64-mingw32          \
        --target=$TARGET                   \
        --prefix="$PREFIX"                 \
        --disable-nls                      \
        --disable-werror                   \
        --enable-languages=c,c++           \
        --with-sysroot="$SYSROOT"
    # Build GCC.
    echo -e "\n\n -> Building GNU Compiler Collection\n\n"
    make all-gcc -j
    make all-target-libgcc -j
    echo -e "\n\n -> Installing GNU Compiler Collection\n\n"
    make install-gcc -j
    make install-target-libgcc -j
    cd ..
fi


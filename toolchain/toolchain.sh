#!/bin/bash
# Dependencies:
# |-- Curl
# |-- GNU Tar (xz-utils)
# `-- GNU Diff/Patch
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $ScriptDirectory
# Download source archives.
curl https://mirrors.kernel.org/gnu/binutils/binutils-2.38.tar.xz \
     --output binutils-2.38.tar.xz
curl https://mirrors.kernel.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz \
     --output gcc-11.2.0.tar.xz
# Create source directories.
mkdir -p binutils-2.38
mkdir -p gcc-11.2.0
# Extract archives into source directories.
tar -xf binutils-2.38.tar.xz \
    -C binutils-2.38
tar -xf gcc-11.2.0.tar.xz \
    -C gcc-11.2.0
# Patch GCC.
cd gcc-11.2.0
patch -s -p0 < $ScriptDirectory/gcc-11.2.0.patch
# Create build directories.
cd $ScriptDirectory
mkdir -p cross
mkdir -p binutils-build
mkdir -p gcc-build
# Set variables.
PREFIX="$ScriptDirectory/cross"
TARGET="x86_64-lensoros-elf"
# Configure binutils.
cd binutils-build
../binutils-2.38/configure \
    --target=$TARGET \
    --prefix="$PREFIX" \
    --with-sysroot \
    --disable-nls \
    --disable-werror
# Build binutils.
make
make install
# Configure GCC.
cd $ScriptDirectory/gcc-build
../gcc-11.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
# Build GCC.
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

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
# Extract archives into source directories.
mkdir -p "binutils-2.38"
tar -xf binutils-2.38.tar.xz -v -C .
mkdir -p "gcc-11.2.0"
tar -xf gcc-11.2.0.tar.xz -v -C .
# Patch GCC.
patch -s -u -p0 < $ScriptDirectory/gcc-11.2.0.patch
# Create build directories.
mkdir -p cross
mkdir -p binutils-build
mkdir -p gcc-build
# Set variables.
PREFIX="$ScriptDirectory/cross"
TARGET="x86_64-lensoros-elf"
# Configure binutils.
cd binutils-build
$ScriptDirectory/binutils-2.38/configure \
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
$ScriptDirectory/gcc-11.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
# Build GCC.
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc


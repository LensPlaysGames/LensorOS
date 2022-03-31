#!/bin/bash
# Dependencies:
# |-- Curl
# |-- GNU Tar (xz-utils)
# `-- GNU Diff/Patch
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $ScriptDirectory
# Download source archives.
echo -e "\n\n -> Downloading GNU Binutils Source Archive\n\n"
curl https://mirrors.kernel.org/gnu/binutils/binutils-2.38.tar.xz \
     --output binutils-2.38.tar.xz
echo -e "\n\n -> Downloading GNU Compiler Collection Source Archive\n\n"
curl https://mirrors.kernel.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz \
     --output gcc-11.2.0.tar.xz
# Extract archives into source directories.
echo -e "\n\n -> Extracting GNU Binutils\n\n"
mkdir -p "binutils-2.38"
tar -xf binutils-2.38.tar.xz -v -C .
echo -e "\n\n -> Extracting GNU Compiler Collection\n\n"
mkdir -p "gcc-11.2.0"
tar -xf gcc-11.2.0.tar.xz -v -C .
# Patch GCC.
echo -e "\n\n -> Patching GNU Compiler Collection\n\n"
patch -s -u -p0 < $ScriptDirectory/gcc-11.2.0.patch
# Create build directories.
mkdir -p cross
mkdir -p binutils-build
mkdir -p gcc-build
# Set variables.
PREFIX="$ScriptDirectory/cross"
TARGET="x86_64-lensoros-elf"
# Configure binutils.
echo -e "\n\n -> Configuring GNU Binutils\n\n"
cd binutils-build
$ScriptDirectory/binutils-2.38/configure \
    --target=$TARGET \
    --prefix="$PREFIX" \
    --with-sysroot \
    --disable-nls \
    --disable-werror
# Build binutils.
echo -e "\n\n -> Building & Installing GNU Binutils\n\n"
make -j
make install -j
# Configure GCC.
echo -e "\n\n -> Configuring GNU Compiler Collection\n\n"
cd $ScriptDirectory/gcc-build
$ScriptDirectory/gcc-11.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
# Build GCC.
echo -e "\n\n -> Building & Installing GNU Compiler Collection\n\n"
make all-gcc -j
make all-target-libgcc -j
make install-gcc -j
make install-target-libgcc -j


#!/bin/bash
# Dependencies:
# |-- Curl
# |-- GNU Tar (xz-utils)
# `-- GNU Diff/Patch
# Set variables.
TARGET="x86_64-lensoros-elf"
PREFIX="$ScriptDirectory/cross"
SYSROOT="$ScriptDirectory/../root"
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Ensure known working directory.
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
# Patch Binutils.
echo -e "\n\n -> Patching GNU Binutils\n\n"
patch -s -u -p0 < $ScriptDirectory/binutils-2.38-lensor.patch
# Patch GCC.
echo -e "\n\n -> Patching GNU Compiler Collection\n\n"
patch -s -u -p0 < $ScriptDirectory/gcc-11.2.0-lensor.patch
# Bootstrap sysroot.
if [[ ! -d "$SYSROOT" ]] ; then
    mkdir -p "$SYSROOT"
	# Copy base filesystem with pre-built libc
	#     binaries into newly-created sysroot.
	# This solves the issue of having to build a
	#     bootstrap version of the compiler first.
	cp -r ../base "$SYSROOT"
	# Use libc install script to install system headers.
	source ../user/libc/install.sh "$SYSROOT"
fi
# Create output build directory.
mkdir -p cross
# Configure binutils.
echo -e "\n\n -> Configuring GNU Binutils\n\n"
mkdir -p binutils-build
cd binutils-build
$ScriptDirectory/binutils-2.38/configure \
    --target=$TARGET \
    --prefix="$PREFIX" \
    --with-sysroot="$SYSROOT" \
    --disable-nls \
    --disable-werror
# Build binutils.
echo -e "\n\n -> Building & Installing GNU Binutils\n\n"
make -j
make install -j
# Configure GCC.
echo -e "\n\n -> Configuring GNU Compiler Collection\n\n"
mkdir -p gcc-build
cd $ScriptDirectory/gcc-build
$ScriptDirectory/gcc-11.2.0/configure \
	--target=$TARGET \
	--prefix="$PREFIX" \
	--disable-nls \
	--enable-languages=c,c++ \
	--with-sysroot="$SYSROOT"
# Build GCC.
echo -e "\n\n -> Building & Installing GNU Compiler Collection\n\n"
make all-gcc -j
make all-target-libgcc -j
make install-gcc -j
make install-target-libgcc -j

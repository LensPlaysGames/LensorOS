#!/bin/bash

#     Dependencies:
#     |-- Curl
#     |-- GNU Diff/Patch
#     |-- GNU Make
#     |-- GNU Tar (xz-utils)
#     `-- x86_64-linux-gnu compiler

# Set variables.
ToolchainDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TARGET="x86_64-lensor"
PREFIX="$ToolchainDir/cross"
SYSROOT="$ToolchainDir/../root"
if [ -z "$BINUTILS_VERSION" ]; then
    BINUTILS_VERSION="2.38";
fi
BINUTILS_STRING="binutils-$BINUTILS_VERSION"
if [ -z "$GCC_VERSION" ]; then
    GCC_VERSION="12.1.0"
fi
GCC_STRING="gcc-$GCC_VERSION"
# Ensure known working directory (assuming script wasn't moved).
cd $ToolchainDir
# Download, extract, and patch source archives if they haven't been already.
if [ ! -d $BINUTILS_STRING ]; then
    echo -e "\n\n -> Downloading GNU Binutils Source Archive\n\n"
    curl https://mirrors.kernel.org/gnu/binutils/$BINUTILS_STRING.tar.xz \
         --output $BINUTILS_STRING.tar.xz
    echo -e "\n\n -> Extracting GNU Binutils\n\n"
    mkdir -p "$BINUTILS_STRING"
    tar -xf $BINUTILS_STRING.tar.xz -C .
    echo -e "\n\n -> Patching GNU Binutils\n\n"
    patch -s -u -p0 < $ToolchainDir/$BINUTILS_STRING-lensor.patch
else
    echo -e "\n\n -> Using existing GNU Binutils Source\n\n"
fi
if [ ! -d $GCC_STRING ]; then
    echo -e "\n\n -> Downloading GNU Compiler Collection Source Archive\n\n"
    curl https://mirrors.kernel.org/gnu/gcc/$GCC_STRING/$GCC_STRING.tar.xz \
         --output $GCC_STRING.tar.xz
    echo -e "\n\n -> Extracting GNU Compiler Collection\n\n"
    mkdir -p "$GCC_STRING"
    tar -xf $GCC_STRING.tar.xz -C .
    echo -e "\n\n -> Downloading GNU Compiler Collection Prerequisites\n\n"
    cd $GCC_STRING
    ./contrib/download_prerequisites
    cd $ToolchainDir
    echo -e "\n\n -> Patching GNU Compiler Collection\n\n"
    patch -s -u -p0 < $ToolchainDir/$GCC_STRING-lensor.patch
else
    echo -e "\n\n -> Using existing GNU Compiler Collection Source\n\n"
fi
if [ ! -d $SYSROOT ] ; then
    cd $ToolchainDir/../scripts
    source sysroot.sh
    cd $ToolchainDir
else
    echo -e "\n\n -> Using existing System Root at $SYSROOT\n\n"
fi
# Create output build directory.
mkdir -p cross
if [ ! -d $BINUTILS_STRING"-build" ] ; then
    echo -e "\n\n -> Configuring GNU Binutils\n\n"
    mkdir -p $BINUTILS_STRING"-build"
    cd $BINUTILS_STRING"-build"
    $ToolchainDir/$BINUTILS_STRING/configure  \
        --target=$TARGET                      \
        --prefix="$PREFIX"                    \
        --with-sysroot="$SYSROOT"             \
        --disable-nls                         \
        --disable-werror
    # Build binutils.
    echo -e "\n\n -> Building & Installing GNU Binutils\n\n"
    make -j
    make install -j
    cd ..
else
    echo -e "\n\n -> Using existing build of GNU Binutils\n\n"
fi
if [ ! -d $GCC_STRING"-build" ] ; then
    echo -e "\n\n -> Configuring GNU Compiler Collection\n\n"
    cd $ToolchainDir
    mkdir -p $GCC_STRING"-build"
    cd $GCC_STRING"-build"
    $ToolchainDir/$GCC_STRING/configure  \
        --target=$TARGET                 \
        --prefix="$PREFIX"               \
        --disable-nls                    \
        --disable-werror                 \
        --enable-languages=c,c++         \
        --with-sysroot="$SYSROOT"
    # Build GCC.
    echo -e "\n\n -> Building & Installing GNU Compiler Collection\n\n"
    make all-gcc -j
    make all-target-libgcc -j
    make install-gcc -j
    make install-target-libgcc -j
    cd ..
else
    echo -e "\n\n -> Using existing build of GNU Compiler Collection\n\n"
fi


cmake_minimum_required(VERSION 3.14)
project(LensorOS_libc VERSION 0.0.1 LANGUAGES ASM C CXX)

# TODO:
# `-- Create target that copies build libraries and system headers to sysroot for cross compilation

# Use cross compiler.
set(CMAKE_ASM_COMPILER x86_64-lensoros-elf-gcc)
set(CMAKE_C_COMPILER   x86_64-lensoros-elf-gcc)
set(CMAKE_CXX_COMPILER x86_64-lensoros-elf-g++)

# Export compilation database in JSON format.
# This gives language servers like `clangd` all the info they need.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(
LIBC_HEADERS
    errno.h
    float.h
    limits.h
    stdalign.h
    stdarg.h
    stdbool.h
    stddef.h
    stdint.h
    stdio.h
    stdlib.h
    stdnoreturn.h
    string.h
    time.h
    unistd.h
)

set(
LIBC_SOURCES
    crt0.s
    crti.s
    crtn.s
    stdlib.cpp
    string.cpp
)

add_library(c ${LIBC_SOURCES} ${LIBC_HEADERS})

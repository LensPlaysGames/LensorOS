cmake_minimum_required(VERSION 3.14)

# TODO:
# `-- Create target that copies built libraries and system headers to sysroot for cross compilation

# Use cross compiler.
set(CMAKE_ASM_COMPILER x86_64-lensoros-elf-gcc)
set(CMAKE_C_COMPILER   x86_64-lensoros-elf-gcc)
set(CMAKE_CXX_COMPILER x86_64-lensoros-elf-g++)

project(LensorOS_libc VERSION 0.0.1 LANGUAGES ASM C CXX)

# Export compilation database in JSON format.
# This gives language servers like `clangd` all the info they need.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_library(c)
target_include_directories(
  c
  PUBLIC
  .
)
target_sources(
  c
  PUBLIC
  crt0.s
  crti.s
  crtn.s
  stdlib.cpp
  string.cpp
  unistd.cpp
)
target_compile_options(
  c
  PRIVATE
  -fno-stack-protector
)

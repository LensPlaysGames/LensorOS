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


cmake_minimum_required(VERSION 3.14)

# TODO:
# `-- Create target that copies built libraries and system headers to sysroot for cross compilation

# Use cross compiler.
set( CMAKE_TOOLCHAIN_FILE "../../kernel/lensor_gnu_toolchain.cmake" )

project(LensorOS_libc VERSION 0.0.1 LANGUAGES ASM CXX)

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
  sys/syscalls.cpp
)
target_compile_options(
  c
  PRIVATE
  -fno-stack-protector
)

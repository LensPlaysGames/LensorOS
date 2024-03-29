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

set(CMAKE_CXX_STANDARD 20)

if (NOT LENSOR_OS_LIBC_BUILD_HOST)
    message(STATUS "Cross compiling LensorOS libc")
    # Use cross compiler.
    set( CMAKE_TOOLCHAIN_FILE ../../toolchain/lensor_gnu_toolchain.cmake )
else()
    message(STATUS "Building LensorOS libc on host")
endif()

project(LensorOS_libc VERSION 0.0.1 LANGUAGES ASM CXX)

# Export compilation database in JSON format.
# This gives language servers like `clangd` all the info they need.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_library(c)

target_include_directories(
  c
  PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}"
  "${CMAKE_CURRENT_LIST_DIR}/../../std/include"
)

target_sources(
  c
  PUBLIC
  crt0.s
  crti.s
  crtn.s
  abi.cpp
  stdio.cpp
  stdlib.cpp
  string.cpp
  unistd.cpp
)

if(USE_DIAGNOSTICS_COLOUR)
    target_compile_options(
        c
        PRIVATE
        -fdiagnostics-color=always
    )
endif()

target_compile_options(
  c
  PRIVATE
  -fPIE
  -Wall
  -Wextra
  -Wconversion
  -pedantic
  -Werror=return-type
  -Wno-unused-parameter
  -Wno-unused-variable
)

target_compile_options(
    c
    PUBLIC
    -fno-stack-protector
    -fno-exceptions
    -nostdlib
    -nostdinc
    -lgcc
    -mno-red-zone
)

target_link_options(
    c
    PUBLIC
    -fno-stack-protector
    -fno-exceptions
    -nostdlib
    -nostdinc
    -lgcc
)

if (LENSOR_OS_LIBC_BUILD_HOST)
    target_compile_options(
        c
        PRIVATE
        -g
        -O0
    )
endif()

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../root")
set(STD "${CMAKE_CURRENT_LIST_DIR}/../../std")

if (NOT LENSOR_OS_LIBC_BUILD_HOST)
  add_custom_target(
    install-libc ALL
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:c> "${ROOT}/lib"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/c.dir/crt0.s.obj" "${ROOT}/lib/crt0.o"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/c.dir/crti.s.obj" "${ROOT}/lib/crti.o"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/c.dir/crtn.s.obj" "${ROOT}/lib/crtn.o"
    COMMAND ${CMAKE_COMMAND} "-DSOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}" -P "${CMAKE_CURRENT_LIST_DIR}/install-headers.cmake"
    COMMENT "Installing LibC headers"

    DEPENDS c
  )
endif()

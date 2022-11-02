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


if (NOT LENSOR_OS_LIBC_BUILD_HOST)
    message(STATUS "Cross compiling LensorOS libc")
    # Use cross compiler.
    set( CMAKE_TOOLCHAIN_FILE "../../kernel/lensor_gnu_toolchain.cmake" )
else()
    message(STATUS "Building LensorOS libc on host")
endif()

project(LensorOS_libc VERSION 0.0.1 LANGUAGES ASM CXX)

# Export compilation database in JSON format.
# This gives language servers like `clangd` all the info they need.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# Check if Enhanced REP MOVSB is supported using asm.
# TODO: Figure out how to check this on Windows.
if(NOT WIN32)
    message(STATUS "Checking if Enhanced REP MOVSB is supported...")
    execute_process(
        COMMAND grep -q "erms" /proc/cpuinfo
        RESULT_VARIABLE ERMS_SUPPORTED
    )

    if(ERMS_SUPPORTED EQUAL 0)
        message(STATUS "Enhanced REP MOVSB is supported.")
        add_compile_definitions(_LIBC_HAVE_ERMS)
    endif()
endif()

add_library(c)
target_compile_features(c PUBLIC cxx_std_17)

target_include_directories(
  c
  PUBLIC
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/../../std/include"
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

target_compile_options(
  c
  PRIVATE
  -fdiagnostics-color=always
  -fPIE
#  -msse2
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

set(ROOT "${PROJECT_SOURCE_DIR}/../../root")
set(STD "${PROJECT_SOURCE_DIR}/../../std")

if (NOT LENSOR_OS_LIBC_BUILD_HOST)
add_custom_target(
  install-libc ALL

  # Archive and object files.
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:c> "${ROOT}/lib"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/CMakeFiles/c.dir/crt0.s.obj" "${ROOT}/lib/crt0.o"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/CMakeFiles/c.dir/crti.s.obj" "${ROOT}/lib/crti.o"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/CMakeFiles/c.dir/crtn.s.obj" "${ROOT}/lib/crtn.o"

  # LibC standard headers.
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/assert.h" "${ROOT}/inc/assert.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/errno.h" "${ROOT}/inc/errno.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/float.h" "${ROOT}/inc/float.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/limits.h" "${ROOT}/inc/limits.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/stdalign.h" "${ROOT}/inc/stdalign.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/stdarg.h" "${ROOT}/inc/stdarg.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/stdbool.h" "${ROOT}/inc/stdbool.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/stdio.h" "${ROOT}/inc/stdio.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/stdlib.h" "${ROOT}/inc/stdlib.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/stdnoreturn.h" "${ROOT}/inc/stdnoreturn.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/string.h" "${ROOT}/inc/string.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/time.h" "${ROOT}/inc/time.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/unistd.h" "${ROOT}/inc/unistd.h"

  # LibC standard headers in nested directories.
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/sys" "${ROOT}/inc/sys"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/bits" "${ROOT}/inc/bits"

  # Standard headers shared with the kernel.
  COMMAND ${CMAKE_COMMAND} -E copy "${STD}/include/stddef.h" "${ROOT}/inc/stddef.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${STD}/include/stdint.h" "${ROOT}/inc/stdint.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${STD}/include/bits/decls.h" "${ROOT}/inc/bits/decls.h"

  DEPENDS c
)
endif()
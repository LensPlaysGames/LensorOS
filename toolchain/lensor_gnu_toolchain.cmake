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
# along with LensorOS. If not, see <https://www.gnu.org/licenses

include( "${CMAKE_CURRENT_LIST_DIR}/config.cmake" )

# Until a LensorOS config is added to the CMake source code,
# then subsequently released, `Generic` is used to avoid errors.
set( CMAKE_SYSTEM_NAME Generic )
set( CMAKE_SYSTEM_VERSION ${LensorOS_VERSION} )

# Set sysroot.
set( CMAKE_SYSROOT "${CMAKE_CURRENT_LIST_DIR}/../root" )
if( NOT EXISTS "${CMAKE_SYSROOT}" )
  message(
    FATAL_ERROR
    "The sysroot at ${CMAKE_SYSROOT} does not exist"
  )
endif()

# Set extra places to look for toolchain executables based on host OS.
set( TOOLCHAIN_DIR ${CMAKE_CURRENT_LIST_DIR} )
if( CMAKE_HOST_WIN32 )
  set( TOOLCHAIN_HINTS ${TOOLCHAIN_DIR}/wincross/bin )
else()
  set( TOOLCHAIN_HINTS ${TOOLCHAIN_DIR}/cross/bin )
endif()

# Look for LensorOS Toolchain executables.
find_program(
  CMAKE_C_COMPILER
  ${ARCH}-lensor-gcc
  HINTS
  ${TOOLCHAIN_HINTS}
  REQUIRED
)
find_program(
  CMAKE_CXX_COMPILER
  ${ARCH}-lensor-g++
  HINTS
  ${TOOLCHAIN_HINTS}
  REQUIRED
)

# TODO: Should we add -mno-red-zone here?

# LensorOS toolchain is GCC 11.2 or greater, so we're good here.
set( CMAKE_CXX_STANDARD 20 )

# Skip compiler tests (hard to run an executable made for another OS).
set( CMAKE_C_COMPILER_WORKS 1 )
set( CMAKE_CXX_COMPILER_WORKS 1 )
# Don't try to link during `try_compile()`, just in case.
set( CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY )

# Look for host programs in the host environment, not the target.
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )

# Do look for libraries and includes in the target sysroot.
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

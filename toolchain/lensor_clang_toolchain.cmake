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

set(TARGET_TRIPLE x86_64-unknown-none-elf)

# Set sysroot.
set( CMAKE_SYSROOT "${CMAKE_CURRENT_LIST_DIR}/../root" )
if( NOT EXISTS "${CMAKE_SYSROOT}" )
  message(
    FATAL_ERROR
    "The sysroot at ${CMAKE_SYSROOT} does not exist"
  )
endif()

# Look for LensorOS Toolchain executables.
find_program(
  CMAKE_C_COMPILER
  clang
  REQUIRED
)
find_program(
  CMAKE_CXX_COMPILER
  clang++
  REQUIRED
)

set(
  USERSPACE_COMPILE_FLAGS
  "-target ${TARGET_TRIPLE} --sysroot=${MY_SYSROOT} -I${CMAKE_CURRENT_LIST_DIR}/../def/"
)

set(CMAKE_C_FLAGS "${USERSPACE_COMPILE_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${USERSPACE_COMPILE_FLAGS}" CACHE STRING "" FORCE)

set( CMAKE_CXX_STANDARD 23 )

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

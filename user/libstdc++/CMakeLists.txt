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

set(CMAKE_CXX_STANDARD 20)

project(LensorOS_libstdc++ VERSION 0.0.1 LANGUAGES CXX)

# Export compilation database in JSON format.
# This gives language servers like `clangd` all the info they need.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_library(stdc++ src/stub.cpp)

target_include_directories(
  stdc++
  PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}"
  "${CMAKE_CURRENT_LIST_DIR}/../../std/include"
)

if(USE_DIAGNOSTICS_COLOUR)
    target_compile_options(
        stdc++
        PRIVATE
        -fdiagnostics-color=always
    )
endif()

target_compile_options(
  stdc++
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
    stdc++
    PRIVATE
    -fno-exceptions
    -fno-rtti
    -nostdlib
    -nostdinc
)

target_link_options(
    stdc++
    PRIVATE
    -fno-exceptions
    -fno-rtti
    -nostdlib
    -nostdinc
)

target_compile_options(
    stdc++
    PUBLIC
    -lgcc
)

target_link_options(
    stdc++
    PUBLIC
    -lgcc
)

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../root")
add_custom_target(
  install-libstdc++ ALL
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:stdc++> "${ROOT}/lib"
  COMMENT "Installing LibStdC++ into ${ROOT}/lib"
  DEPENDS stdc++
)

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

set(CMAKE_CXX_STANDARD 23)

project(LensorOS_libguiclient VERSION 0.0.1 LANGUAGES CXX)

# Export compilation database in JSON format.
# This gives language servers like `clangd` all the info they need.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_library(
  guiclient
  src/startup_teardown.cpp
  src/event.cpp
)

target_include_directories(
  guiclient
  PUBLIC
  ../../def/
  inc/
)

target_compile_options(
  guiclient
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

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../root")
add_custom_target(
  install-libguiclient ALL
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:guiclient> "${ROOT}/lib"
  COMMENT "Installing LibGuiClient into ${ROOT}/lib"
  DEPENDS guiclient
)
add_custom_target(
  install-libguiclient-headers ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/inc/" "${ROOT}/inc/"
  COMMENT "Installing LibGuiClient headers into ${ROOT}/inc"
)

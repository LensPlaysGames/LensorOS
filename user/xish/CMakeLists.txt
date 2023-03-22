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


cmake_minimum_required( VERSION 3.14 )
set( xish_VERSION 0.0.1 )
set( xish_LANGUAGES CXX )

# Export compilation database in JSON format.
set( CMAKE_EXPORT_COMPILE_COMMANDS on )

project( xish VERSION ${xish_VERSION} LANGUAGES ${xish_LANGUAGES} )

add_executable( xish src/main.cpp )
target_compile_options(
  xish
  PUBLIC
  -fno-stack-protector
  -fno-exceptions
  -fno-rtti
)
target_link_options(
  xish
  PUBLIC
  -fno-stack-protector
  -fno-exceptions
  -fno-rtti
)

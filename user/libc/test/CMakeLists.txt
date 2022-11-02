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
project(libc-tests C CXX ASM)

# Don't build LibC for LensorOS.
set(LENSOR_OS_LIBC_BUILD_HOST 1)

# Build libc.
add_subdirectory(.. libc)

# Build the tests.
add_executable(test test.c)
target_compile_options(
    test
    PRIVATE
    -Wall -Wextra -Werror=return-type
    -Wno-unused-parameter -Wno-unused-variable
    -O0 -g
)
target_link_libraries(test PRIVATE c)


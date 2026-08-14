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
if(NOT DEFINED PROJECT_ROOT)
    message(FATAL_ERROR "PROJECT_ROOT variable is not set!")
endif()

set(SYSTEM_ROOT "${PROJECT_ROOT}/root")

message(STATUS "\n\n -> Bootstrapping System Root at ${SYSTEM_ROOT}\n\n")

# run mkdir -p "$SystemRoot"
file(MAKE_DIRECTORY "${SYSTEM_ROOT}")

# run cp -r ../base/* "$SystemRoot/"
# Note: CMake's file(COPY) acts like cp -r when targeting directories
if(EXISTS "${PROJECT_ROOT}/base")
    file(COPY "${PROJECT_ROOT}/base/" DESTINATION "${SYSTEM_ROOT}")
endif()

# Like 'find ./ -name "*.h"' -- recursive copy with parent paths
set(LIBC_SRC_DIR "${PROJECT_ROOT}/user/libc")
if(EXISTS "${LIBC_SRC_DIR}")
    # Recursively find all .h files
    file(GLOB_RECURSE LIBC_HEADERS RELATIVE "${LIBC_SRC_DIR}" "${LIBC_SRC_DIR}/*.h")

    foreach(HEADER ${LIBC_HEADERS})
        get_filename_component(HEADER_DIR "${HEADER}" DIRECTORY)
        # Recreate parent paths seamlessly under sysroot/inc
        file(MAKE_DIRECTORY "${SYSTEM_ROOT}/inc/${HEADER_DIR}")
        file(COPY "${LIBC_SRC_DIR}/${HEADER}" DESTINATION "${SYSTEM_ROOT}/inc/${HEADER_DIR}")
    endforeach()
endif()

# run cp "$ProjectRoot"/std/include/* "$SystemRoot/inc/"
# run cp "$ProjectRoot"/std/include/bits/* "$SystemRoot/inc/bits/"
if(EXISTS "${PROJECT_ROOT}/std/include/")
    file(COPY "${PROJECT_ROOT}/std/include/" DESTINATION "${SYSTEM_ROOT}/inc/")
endif()

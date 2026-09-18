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
# along with LensorOS. If not, see <https://www.gnu.org/licenses/>

function(install_headers)
    set(dest_dir ${SOURCE_DIR}/../../root/inc)

    file(GLOB libc_headers        "${SOURCE_DIR}/*.h")
    file(GLOB libc_bits_headers   "${SOURCE_DIR}/bits/*.h")
    file(GLOB libc_sys_headers    "${SOURCE_DIR}/sys/*.h")

    file(COPY ${libc_headers}        DESTINATION "${dest_dir}")
    file(COPY ${libc_bits_headers}   DESTINATION "${dest_dir}/bits")
    file(COPY ${libc_sys_headers}    DESTINATION "${dest_dir}/sys")
endfunction()

install_headers()

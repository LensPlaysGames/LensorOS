#!/bin/bash

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


ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BuildDirectory="$ScriptDirectory/../kernel/bin"

run() {
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

# TODO: Think about how to run LensorOS from a bootable ISO image while
# also having the data partition loaded on there.

run $ScriptDirectory/mkimg.sh
run mkdir -p $BuildDirectory/iso
run cp \
    $BuildDirectory/LensorOS.img \
    $BuildDirectory/iso
run xorriso \
    -as mkisofs \
    -R -f \
    -e LensorOS.img \
    -no-emul-boot \
    -o $BuildDirectory/LensorOS.iso \
    $BuildDirectory/iso
run rm -r $BuildDirectory/iso

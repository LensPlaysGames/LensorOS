#!/bin/bash
# Copyright 2022, Contributors To LensorOS.
# All rights reserved.

# This file is part of LensorOS.

# LensorOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# LensorOS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with LensorOS. If not, see <https://www.gnu.org/licenses/>.

ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BuildDirectory="$ScriptDirectory/../kernel/bin"

run(){
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

EFISystemPartitionOffset=2048
FATImageSectorCount=93750

# Generate FAT32 UEFI-compatible boot image.
run $ScriptDirectory/mkimg.sh

# Create an empty file that's 48MiB in size.
run cd $BuildDirectory
run dd if=/dev/zero of=LensorOS.bin count=96000

# Use fdisk to create a new GPT with an EFI System partition.
echo "g
n
1
$EFISystemPartitionOffset
+$FATImageSectorCount
t
1
w
" | fdisk LensorOS.bin

# Copy FAT32 boot image into disk image at EFI System partition offset.
run dd if=LensorOS.img of=LensorOS.bin \
    bs=512 \
    count=$FATImageSectorCount \
    skip=0 \
    seek=$EFISystemPartitionOffset \
    conv=notrunc

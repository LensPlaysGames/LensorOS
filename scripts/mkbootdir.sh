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
RepositoryDirectory="$ScriptDirectory/.."
BootloaderEFI="$RepositoryDirectory/gnu-efi/x86_64/bootloader/main.efi"
BuildDirectory="$RepositoryDirectory/kernel/bin"

run(){
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

# Ensure existence of files used.
if [ ! -f "$BootloaderEFI" ]; then
    echo -e "\n\n -> ERROR: Could not locate bootloader executable at $BootloaderEFI\n\n"
    exit 1
fi
if [ ! -f "$BuildDirectory/kernel.elf" ]; then
    echo -e "\n\n -> ERROR: Could not locate kernel executable at $BuildDirectory/kernel.elf\n\n"
    exit 1
fi
if [ ! -f "$BuildDirectory/dfltfont.psf" ]; then
    echo -e "\n\n -> ERROR: Could not locate font at $BuildDirectory/dfltfont.psf\n\n"
    exit 1
fi
if [ ! -f "$ScriptDirectory/startup.nsh" ]; then
    echo -e "\n\n -> ERROR: Could not locate UEFI startup script at $ScriptDirectory/startup.nsh\n\n"
    exit 1
fi

if [ -d "$RepositoryDirectory/boot" ]; then
    cd $RepositoryDirectory
    rm -rf boot/
fi

cd $RepositoryDirectory
mkdir -p boot/EFI/BOOT
mkdir -p boot/LensorOS
cp $ScriptDirectory/startup.nsh boot/
cp $BootloaderEFI boot/EFI/BOOT/
cp $BuildDirectory/kernel.elf boot/LensorOS/
cp $BuildDirectory/dfltfont.psf boot/LensorOS/

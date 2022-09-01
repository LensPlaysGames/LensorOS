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
OVMFDirectory="$ScriptDirectory/../OVMFbin"
if ! [ -f $BuildDirectory/LensorOS.img ] ; then
	echo -e "\n\n -> ERROR: Couldn't locate $BuildDirectory/LensorOS.img\n\n"
	exit 1
fi
if ! [ -f $OVMFDirectory/OVMF_CODE-pure-efi.fd ] ; then
	echo -e "\n\n -> ERROR: Couldn't locate $OVMFDirectory/OVMF_CODE-pure-efi.fd\n\n"
	exit 1
fi
if ! [ -f $OVMFDirectory/OVMF_VARS_LensorOS.fd ] ; then
	if ! [ -f $OVMFDirectory/OVMF_VARS-pure-efi.fd ] ; then
		echo -e "\n\n -> ERROR: Couldn't locate $OVMFDirectory/OVMF_VARS-pure-efi.fd\n\n"
		exit 1
	fi
	cp $OVMFDirectory/OVMF_VARS-pure-efi.fd $OVMFDirectory/OVMF_VARS_LensorOS.fd
fi
qemu-system-x86_64 \
    -s -S \
    -d cpu_reset \
    -machine q35 \
    -cpu qemu64 \
    -m 100M \
    -serial stdio \
    -soundhw pcspk \
    -rtc base=localtime,clock=host,driftfix=none \
    -hda $BuildDirectory/LensorOS.bin \
    -drive if=pflash,format=raw,unit=0,file=$OVMFDirectory/OVMF_CODE-pure-efi.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=$OVMFDirectory/OVMF_VARS_LensorOS.fd \
	-net none

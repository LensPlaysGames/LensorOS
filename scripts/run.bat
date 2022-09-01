:: Copyright 2022, Contributors To LensorOS.
:: All rights reserved.

:: This file is part of LensorOS.

:: LensorOS is free software: you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation, either version 3 of the License, or
:: (at your option) any later version.

:: LensorOS is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
:: GNU General Public License for more details.

:: You should have received a copy of the GNU General Public License
:: along with LensorOS. If not, see <https://www.gnu.org/licenses/>. 
SET OVMFbin=%0/../../OVMFbin
SET BuildDirectory=%0/../../kernel/bin
SET OVMFbin=%OVMFbin:"=%
SET BuildDirectory=%BuildDirectory:"=%
IF NOT EXIST %BuildDirectory%/LensorOS.img echo " -> ERROR: Could not find %BuildDirectory%/LensorOS.img" & EXIT
IF NOT EXIST %OVMFbin%/OVMF_CODE-pure-efi.fd echo " -> ERROR: Could not find %OVMFbin%/OVMF_CODE-pure-efi.fd" & EXIT
IF NOT EXIST %OVMFbin%/OVMF_VARS_LensorOS.fd cp %OVMFbin%/OVMF_VARS-pure-efi.fd %OVMFbin%/OVMF_VARS_LensorOS.fd
qemu-system-x86_64 ^
 -d cpu_reset ^
 -machine q35 ^
 -cpu qemu64 ^
 -m 100M ^
 -serial stdio ^
 -soundhw pcspk ^
 -rtc base=localtime,clock=host,driftfix=none ^
 -net none ^
 -drive format=raw,file=%BuildDirectory%/LensorOS.img ^
 -drive if=pflash,format=raw,unit=0,file=%OVMFbin%/OVMF_CODE-pure-efi.fd,readonly=on ^
 -drive if=pflash,format=raw,unit=1,file=%OVMFbin%/OVMF_VARS_LensorOS.fd

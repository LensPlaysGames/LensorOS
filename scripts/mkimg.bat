:: Copyright 2022, Contributors To LensorOS.
:: All rights reserved.
::
:: This file is part of LensorOS.
::
:: LensorOS is free software: you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation, either version 3 of the License, or
:: (at your option) any later version.
::
:: LensorOS is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with LensorOS. If not, see <https://www.gnu.org/licenses/>.
SET ScriptDirectory=%0/..
SET RepositoryDirectory=%ScriptDirectory%/..
SET BootloaderEFI=%RepositoryDirectory%/gnu-efi/x86_64/bootloader/main.efi
SET BuildDirectory=%RepositoryDirectory%/kernel/bin
SET ScriptDirectory=%ScriptDirectory:"=%
SET RepositoryDirectory=%RepositoryDirectory:"=%
SET BootloaderEFI=%BootloaderEFI:"=%
SET BuildDirectory=%BuildDirectory:"=%
md %BuildDirectory%
dd if=/dev/zero of=%BuildDirectory%/LensorOS.img count=93750
mformat -i %BuildDirectory%/LensorOS.img -F -v "EFI System" ::
@echo " -> Created FAT32 UEFI bootable disk image"
mmd -i %BuildDirectory%/LensorOS.img ::/EFI
mmd -i %BuildDirectory%/LensorOS.img ::/EFI/BOOT
mmd -i %BuildDirectory%/LensorOS.img ::/LensorOS
@echo " -> Directories initialized"
mcopy -i %BuildDirectory%/LensorOS.img %BootloaderEFI% ::/EFI/BOOT
mcopy -i %BuildDirectory%/LensorOS.img %ScriptDirectory%/startup.nsh ::
mcopy -i %BuildDirectory%/LensorOS.img %BuildDirectory%/kernel.elf ::/LensorOS
mcopy -i %BuildDirectory%/LensorOS.img %BuildDirectory%/dfltfont.psf ::/LensorOS
@echo " -> Resources copied"

dd if=/dev/zero of=%BuildDirectory%/LensorOSData.img count=93750
mformat -i %BuildDirectory%/LensorOSData.img -F -v "LensorOS" ::
@echo " -> Created FAT32 LensorOSData image"
mmd -i %BuildDirectory%/LensorOSData.img ::/bin
mmd -i %BuildDirectory%/LensorOSData.img ::/res
mmd -i %BuildDirectory%/LensorOSData.img ::/res/fonts
mmd -i %BuildDirectory%/LensorOSData.img ::/res/fonts/psf1
@echo "\n\n -> Directories initialized\n\n"
mcopy -i %BuildDirectory%/LensorOSData.img %BuildDirectory%/dfltfont.psf ::/res/fonts/psf1
mcopy -i %BuildDirectory%/LensorOSData.img %RepositoryDirectory%/user/blazeit/blazeit ::/bin
mcopy -i %BuildDirectory%/LensorOSData.img %RepositoryDirectory%/user/stdout/stdout ::/bin
:: TODO get rid of following lines once directory traversal works in FAT driver...
mcopy -i %BuildDirectory%/LensorOSData.img %BuildDirectory%/dfltfont.psf ::
mcopy -i %BuildDirectory%/LensorOSData.img %RepositoryDirectory%/user/stdout/stdout ::
mcopy -i %BuildDirectory%/LensorOSData.img %RepositoryDirectory%/user/blazeit/blazeit ::
@echo "\n\n -> Resources copied\n\n"

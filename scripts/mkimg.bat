SET ScriptDirectory=%0/..
SET RepositoryDirectory=%ScriptDirectory%/..
SET BootloaderEFI=%RepositoryDirectory%/gnu-efi/x86_64/bootloader/main.efi
SET BuildDirectory=%RepositoryDirectory%/kernel/bin
SET ScriptDirectory=%ScriptDirectory:"=%
SET RepositoryDirectory=%RepositoryDirectory:"=%
SET BootloaderEFI=%BootloaderEFI:"=%
SET BuildDirectory=%BuildDirectory:"=%
mkdir -p %BuildDirectory%
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

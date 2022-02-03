#!/bin/sh

OSName="LensorOS"
BuildDirectory="bin"
BootloaderEFI="../gnu-efi/x86_64/bootloader/main.efi"

run(){
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

run mkdir -p $BuildDirectory
run dd if=/dev/zero of=$BuildDirectory/$OSName.img count=93750
run mformat -i $BuildDirectory/$OSName.img -F -v "bootdisk" ::
echo "    Created FAT32 bootable disk image."
run mmd -i $BuildDirectory/$OSName.img ::/EFI
run mmd -i $BuildDirectory/$OSName.img ::/EFI/BOOT
run mmd -i $BuildDirectory/$OSName.img ::/LensorOS
echo "    Directories initialized."
run mcopy -i $BuildDirectory/$OSName.img $BootloaderEFI ::/EFI/BOOT
run mcopy -i $BuildDirectory/$OSName.img startup.nsh ::
run mcopy -i $BuildDirectory/$OSName.img $BuildDirectory/kernel.elf ::/LensorOS
run mcopy -i $BuildDirectory/$OSName.img $BuildDirectory/dfltfont.psf ::/LensorOS
echo "    Resources copied."

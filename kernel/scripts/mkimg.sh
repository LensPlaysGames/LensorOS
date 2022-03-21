#!/bin/bash
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
KernelDirectory="$ScriptDirectory/.."
BuildDirectory="$KernelDirectory/bin"
BootloaderEFI="$KernelDirectory/../gnu-efi/x86_64/bootloader/main.efi"

run(){
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

run mkdir -p $BuildDirectory
run dd if=/dev/zero of=$BuildDirectory/LensorOS.img count=93750
run mformat -i $BuildDirectory/LensorOS.img -F -v "EFI System" ::
echo "    Created FAT32 bootable disk image."
run mmd -i $BuildDirectory/LensorOS.img ::/EFI
run mmd -i $BuildDirectory/LensorOS.img ::/EFI/BOOT
run mmd -i $BuildDirectory/LensorOS.img ::/LensorOS
echo "    Directories initialized."
run mcopy -i $BuildDirectory/LensorOS.img $BootloaderEFI ::/EFI/BOOT
run mcopy -i $BuildDirectory/LensorOS.img $ScriptDirectory/startup.nsh ::
run mcopy -i $BuildDirectory/LensorOS.img $BuildDirectory/kernel.elf ::/LensorOS
run mcopy -i $BuildDirectory/LensorOS.img $BuildDirectory/dfltfont.psf ::/LensorOS
echo "    Resources copied."

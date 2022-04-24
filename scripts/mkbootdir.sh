#!/bin/bash
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

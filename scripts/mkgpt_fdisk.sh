#!/bin/bash
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

#!/bin/bash
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
KernelDirectory="$ScriptDirectory/.."
OVMFDirectory="$KernelDirectory/../OVMFbin"
if ! [ -f $OVMFDirectory/OVMF_VARS_LensorOS.fd ] ; then
    cp $OVMFDirectory/OVMF_VARS-pure-efi.fd $OVMFDirectory/OVMF_VARS_LensorOS.fd
fi
qemu-system-x86_64 \
    -d cpu_reset \
    -machine q35 \
    -cpu qemu64 \
    -m 100M \
    -serial stdio \
    -vga cirrus \
    -soundhw pcspk \
    -rtc base=localtime,clock=host,driftfix=none \
    -hda $KernelDirectory/bin/LensorOS.bin \
    -drive if=pflash,format=raw,unit=0,file=$OVMFDirectory/OVMF_CODE-pure-efi.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=$OVMFDirectory/OVMF_VARS_LensorOS.fd -net none

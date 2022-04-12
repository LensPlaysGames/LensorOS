#!/bin/bash
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BuildDirectory="$ScriptDirectory/../kernel/bin"
OVMFDirectory="$ScriptDirectory/../OVMFbin"
qemu-system-x86_64 \
    -s -S \
    -d cpu_reset \
    -machine q35 \
    -cpu qemu64 \
    -m 100M \
    -serial stdio \
    -soundhw pcspk \
    -net none \
    -rtc base=localtime,clock=host,driftfix=none \
    -drive format=raw,file=$BuildDirectory/LensorOS.img \
    -drive if=pflash,format=raw,unit=0,file=$OVMFDirectory/OVMF_CODE-pure-efi.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=$OVMFDirectory/OVMF_VARS-pure-efi.fd

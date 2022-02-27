#!/bin/bash
OSName="LensorOS"
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BuildDirectory="$ScriptDirectory/bin"
OVMFDirectory="$ScriptDirectory/../OVMFbin"
qemu-system-x86_64 -s -S -cpu qemu64 -m 100M -rtc base=localtime,clock=host,driftfix=none -machine q35 -serial stdio -vga cirrus -soundhw pcspk -d cpu_reset -drive format=raw,file=$BuildDirectory/$OSName.img -drive if=pflash,format=raw,unit=0,file=$OVMFDirectory/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$OVMFDirectory/OVMF_VARS-pure-efi.fd -net none

SET OVMFbin=%0/../../OVMFbin
SET BuildDirectory=%0/../../kernel/bin
SET OVMFbin=%OVMFbin:"=%
SET BuildDirectory=%BuildDirectory:"=%
IF NOT EXIST %BuildDirectory%/LensorOS.img echo " -> ERROR: Could not find %BuildDirectory%/LensorOS.img" & EXIT
IF NOT EXIST %OVMFbin%/OVMF_CODE-pure-efi.fd echo " -> ERROR: Could not find %OVMFbin%/OVMF_CODE-pure-efi.fd" & EXIT
IF NOT EXIST %OVMFbin%/OVMF_VARS_LensorOS.fd cp %OVMFbin%/OVMF_VARS-pure-efi.fd %OVMFbin%/OVMF_VARS_LensorOS.fd
qemu-system-x86_64 ^
 -s -S ^
 -d cpu_reset ^
 -machine q35 ^
 -cpu qemu64 ^
 -m 100M ^
 -serial stdio ^
 -soundhw pcspk ^
 -net none ^
 -rtc base=localtime,clock=host,driftfix=none ^
 -drive format=raw,file=%BuildDirectory%/LensorOS.img ^
 -drive if=pflash,format=raw,unit=0,file=%OVMFbin%/OVMF_CODE-pure-efi.fd,readonly=on ^
 -drive if=pflash,format=raw,unit=1,file=%OVMFbin%/OVMF_VARS_LensorOS.fd

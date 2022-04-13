SET OVMFbin=%0/../../OVMFbin
SET BuildDirectory=%0/../../kernel/bin
SET OVMFbin=%OVMFbin:"=%
SET BuildDirectory=%BuildDirectory:"=%
IF NOT EXIST %BuildDirectory%/LensorOS.img echo " -> ERROR: Could not find %BuildDirectory%/LensorOS.img" & EXIT
IF NOT EXIST %OVMFbin%/OVMF_CODE-pure-efi.fd echo " -> ERROR: Could not find %OVMFbin%/OVMF_CODE-pure-efi.fd" & EXIT
IF NOT EXIST %OVMFbin%/OVMF_VARS_LensorOS.fd cp %OVMFbin%/OVMF_VARS-pure-efi.fd %OVMFbin%/OVMF_VARS_LensorOS.fd
qemu-system-x86_64.exe ^
 -cpu qemu64 ^
 -machine q35 ^
 -m 100M ^
 -rtc base=localtime,clock=host,driftfix=none ^
 -serial stdio ^
 -soundhw pcspk ^
 -d cpu_reset ^
 -hda %BuildDirectory%\LensorOS.bin ^
 -drive if=pflash,format=raw,unit=0,file=%OVMFbin%\OVMF_CODE-pure-efi.fd,readonly=on ^
 -drive if=pflash,format=raw,unit=1,file=%OVMFbin%\OVMF_VARS_LensorOS.fd ^
 -net none

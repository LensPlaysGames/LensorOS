set OSNAME=LensorOS
set BUILDDIR=%0/../bin
set OVMFDIR=%0/../../OVMFbin

set BUILDDIR=%BUILDDIR:"=%
set OVMFDIR=%OVMFDIR:"=%
qemu-system-x86_64 -vga cirrus -rtc base=localtime,clock=host,driftfix=none -drive file=%BUILDDIR%/%OSNAME%.img -m 420M -cpu qemu64 -drive if=pflash,format=raw,unit=0,file=%OVMFDIR%/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=%OVMFDIR%/OVMF_VARS-pure-efi.fd -net none
pause

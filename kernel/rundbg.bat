set OSNAME=LensorOS
set BUILDDIR=%0/../bin
set OVMFDIR=%0/../../OVMFbin

set BUILDDIR=%BUILDDIR:"=%
set OVMFDIR=%OVMFDIR:"=%
qemu-system-x86_64 -s -S -m 100M -cpu qemu64 -vga cirrus -rtc base=localtime,clock=host,driftfix=none -machine q35 -net none -drive format=raw,file=%BUILDDIR%/%OSNAME%.img -drive if=pflash,format=raw,unit=0,file=%OVMFDIR%/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=%OVMFDIR%/OVMF_VARS-pure-efi.fd
pause

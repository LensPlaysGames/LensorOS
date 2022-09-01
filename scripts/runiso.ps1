# Copyright 2022, Contributors To LensorOS.
# All rights reserved.

# This file is part of LensorOS.

# LensorOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# LensorOS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with LensorOS. If not, see <https://www.gnu.org/licenses/>. 

$ScriptDirectory = $MyInvocation.MyCommand.Path
if (-not($ScriptDirectory))
{
    Write-Host "Could not get script directory"
    Exit 1
}

$ScriptDirectory = Split-Path -Path $ScriptDirectory
$RepoDirectory = Split-Path -Path $ScriptDirectory
$BuildDirectory = "$RepoDirectory\kernel\bin"
$OVMFDirectory = "$RepoDirectory\OVMFbin"

if (-not(Test-Path -Path "$BuildDirectory\LensorOS.iso"))
{
    Write-Host "Could not find $BuildDirectory\LensorOS.iso"
    Exit 1
}

if (-not(Test-Path -Path "$OVMFDirectory\OVMF_CODE-pure-efi.fd"))
{
    Write-Host "Could not find $OVMFDirectory\OVMF_CODE-pure-efi.fd"
    Exit 1
}

if (-not(Test-Path -Path "$OVMFDirectory\OVMF_VARS_LensorOS.fd"))
{
    if (-not(Test-Path -Path "$OVMFDirectory\OVMF_VARS-pure-efi.fd"))
    {
        Write-Host "Could not find $OVMFDirectory\OVMF_VARS-pure-efi.fd"
        Exit 1
    }
    Copy-Item `
        -Path $OVMFDirectory\OVMF_VARS-pure-efi.fd `
        -Destination $OVMFDirectory\OVMF_VARS_LensorOS.fd
}

qemu-system-x86_64                                                                        `
 -cpu qemu64                                                                              `
 -machine q35                                                                             `
 -m 100M                                                                                  `
 -rtc base=localtime,clock=host,driftfix=none                                             `
 -serial stdio                                                                            `
 -soundhw pcspk                                                                           `
 -d cpu_reset                                                                             `
 -drive format=raw,file=$BuildDirectory\LensorOS.iso,media=cdrom                          `
 -drive if=pflash,format=raw,unit=0,file=$OVMFDirectory\OVMF_CODE-pure-efi.fd,readonly=on `
 -drive if=pflash,format=raw,unit=1,file=$OVMFDirectory\OVMF_VARS_LensorOS.fd             `
 -net none

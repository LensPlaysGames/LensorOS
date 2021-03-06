$ScriptDirectory = $MyInvocation.MyCommand.Path
if (-not($ScriptDirectory))
{
    Write-Host "`nCould not get script directory`n"
    Exit 1
}
$ScriptDirectory = Split-Path -Path $ScriptDirectory
$RepositoryDirectory = Split-Path -Path $ScriptDirectory
$BootloaderEFI = "$RepositoryDirectory\gnu-efi\x86_64\bootloader\main.efi"
$BuildDirectory = "$RepositoryDirectory\kernel\bin"

if (-not(Get-Command -CommandType Application dd))
{
    Write-Host "`nDid not find proper dependencies: dd`n"
	Exit 1
}
if (-not(Get-Command -CommandType Application mtools))
{
    Write-Host "`nDid not find proper dependencies: mtools`n"
	Exit 1
}

New-Item $BuildDirectory -ItemType directory -Force
dd if=/dev/zero of=$BuildDirectory/LensorOS.img count=93750
mformat -i $BuildDirectory/LensorOS.img -F -v "EFI System" ::
Write-Host "`n -> Created FAT32 UEFI bootable disk image`n"
mmd -i $BuildDirectory/LensorOS.img ::/EFI
mmd -i $BuildDirectory/LensorOS.img ::/EFI/BOOT
mmd -i $BuildDirectory/LensorOS.img ::/LensorOS
Write-Host "`n -> Directories initialized`n"
mcopy -i $BuildDirectory/LensorOS.img $BootloaderEFI ::/EFI/BOOT
mcopy -i $BuildDirectory/LensorOS.img $ScriptDirectory/startup.nsh ::
mcopy -i $BuildDirectory/LensorOS.img $BuildDirectory/kernel.elf ::/LensorOS
mcopy -i $BuildDirectory/LensorOS.img $BuildDirectory/dfltfont.psf ::/LensorOS
Write-Host "`n -> Resources copied`n"

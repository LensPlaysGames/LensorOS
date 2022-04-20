# From PowerShell, run with `& .\run.ps1`.
# If you get security errors, use `Set-ExecutionPolicy Bypass -Scope Process`
# to allow scripts to be run for the lifetime of the current powershell process.

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

if (-not(Test-Path -Path "$BuildDirectory\LensorOS.img"))
{
    Write-Host "Could not find $BuildDirectory\LensorOS.img"
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

qemu-system-x86_64																			 `
    -d cpu_reset                                                                             `
    -machine q35                                                                             `
    -cpu qemu64                                                                              `
    -m 100M                                                                                  `
    -serial stdio                                                                            `
    -soundhw pcspk                                                                           `
    -net none                                                                                `
    -rtc base=localtime,clock=host,driftfix=none                                             `
    -drive format=raw,file=$BuildDirectory\LensorOS.img                                      `
    -drive if=pflash,format=raw,unit=0,file=$OVMFDirectory\OVMF_CODE-pure-efi.fd,readonly=on `
    -drive if=pflash,format=raw,unit=1,file=$OVMFDirectory\OVMF_VARS_LensorOS.fd
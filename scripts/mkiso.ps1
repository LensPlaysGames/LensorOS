#Copyright 2022, Contributors To LensorOS.
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
if (-not(Get-Command -CommandType Application xorriso))
{
    Write-Host "`nDid not find proper dependencies: xorriso`n"
    Exit 1
}
$prevPwd = $PWD; Set-Location -ErrorAction Stop -LiteralPath $PSScriptRoot
try {
    $ScriptDirectory = $MyInvocation.MyCommand.Path
    if (-not($ScriptDirectory))
    {
        Write-Host "`nCould not get script directory`n"
        Exit 1
    }
    $ScriptDirectory = Split-Path -Path $ScriptDirectory
    $RepositoryDirectory = Split-Path -Path $ScriptDirectory
    $BuildDirectory = "$RepositoryDirectory\kernel\bin"

    & "$ScriptDirectory\mkimg.ps1"
    Set-Location $BuildDirectory
    New-Item -Path iso -ItemType directory -Force
    Copy-Item `
      -Path .\LensorOS.img `
      -Destination iso\LensorOS.img
    xorriso -as mkisofs -R -f         `
      -e LensorOS.img                 `
      -no-emul-boot                   `
      -o $BuildDirectory\LensorOS.iso `
      iso
    Remove-Item -Path $BuildDirectory\iso -Recurse
    Write-Host "`n -> Created El-Torito ISO-9660 boot media image`n"
}
finally {
    Set-Location $prevPwd
}

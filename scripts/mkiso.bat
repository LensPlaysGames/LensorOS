:: Copyright 2022, Contributors To LensorOS.
:: All rights reserved.

:: This file is part of LensorOS.

:: LensorOS is free software: you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation, either version 3 of the License, or
:: (at your option) any later version.

:: LensorOS is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
:: GNU General Public License for more details.

:: You should have received a copy of the GNU General Public License
:: along with LensorOS. If not, see <https://www.gnu.org/licenses/>. 

SET ScriptDirectory=%0/..
SET BuildDirectory=%ScriptDirectory%/../kernel/bin
SET ScriptDirectory=%ScriptDirectory:"=%
SET BuildDirectory=%BuildDirectory:"=%
cmd /c %ScriptDirectory%/mkimg.bat
cd %BuildDirectory%
md iso
cp LensorOS.img iso/
xorriso -as mkisofs -R -f -e LensorOS.img -no-emul-boot -o LensorOS.iso iso
@echo " -> Created El-Torito ISO-9660 boot media image"
rm -r iso

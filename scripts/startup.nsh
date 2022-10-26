# Copyright 2022, Contributors To LensorOS.
# All rights reserved.
#
# This file is part of LensorOS.
#
# LensorOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LensorOS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LensorOS. If not, see <https://www.gnu.org/licenses/>.


@echo -off
mode 80 25

cls
if exist .\efi\boot\main.efi then
 .\efi\boot\main.efi
 goto END
endif

if exist fs0:\efi\boot\main.efi then
 fs0:
 echo Found bootloader on fs0:
 efi\boot\main.efi
 goto END
endif

if exist fs1:\efi\boot\main.efi then
 fs1:
 echo Found bootloader on fs1:
 efi\boot\main.efi
 goto END
endif

if exist fs2:\efi\boot\main.efi then
 fs2:
 echo Found bootloader on fs2:
 efi\boot\main.efi
 goto END
endif

if exist fs3:\efi\boot\main.efi then
 fs3:
 echo Found bootloader on fs3:
 efi\boot\main.efi
 goto END
endif

if exist fs4:\efi\boot\main.efi then
 fs4:
 echo Found bootloader on fs4:
 efi\boot\main.efi
 goto END
endif

if exist fs5:\efi\boot\main.efi then
 fs5:
 echo Found bootloader on fs5:
 efi\boot\main.efi
 goto END
endif

if exist fs6:\efi\boot\main.efi then
 fs6:
 echo Found bootloader on fs6:
 efi\boot\main.efi
 goto END
endif

if exist fs7:\efi\boot\main.efi then
 fs7:
 echo Found bootloader on fs7:
 efi\boot\main.efi
 goto END
endif

 echo "Unable to find bootloader".
 
:END

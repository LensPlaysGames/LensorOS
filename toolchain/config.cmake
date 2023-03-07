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
# along with LensorOS. If not, see <https://www.gnu.org/licenses

# LensorOS Build Configuration
# Settings and Options

# Settings and options may be set during project configuration, like so:
# cmake -G <generator> -S <srcdir> -B <blddir> -DMACHINE="QEMU"

set(
  ARCH "x86_64"
  CACHE STRING
  "The CPU architecture that LensorOS will be run on"
)
set_property(
  CACHE ARCH
  PROPERTY STRINGS
  "x86_64"
)
set(
  MACHINE "VBOX"
  CACHE STRING
  "The machine type that LensorOS will run on"
)
set_property(
  CACHE MACHINE
  PROPERTY STRINGS
  "PC"
  "QEMU"
  "VBOX"
  "VMWARE"
)
option(
  HIDE_UART_COLOR_CODES
  "Do not print ANSI terminal color codes to serial output.
Particularly useful if the terminal you are using does not support it.
On by default for compatibility reasons."
  ON
)
option(
  QEMU_DEBUG
  "Start QEMU with `-S -s` flags, halting startup until a debugger has been attached."
  OFF
)

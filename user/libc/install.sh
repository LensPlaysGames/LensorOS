#!/bin/bash

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

if [ -z $1 ]; then
    echo "No target directory given. Please specify the sysroot directory as an argument on the command line."
    exit 1
fi
echo "Copying headers to $1/usr/include"
find ./ -name '*.h' -exec cp --parents {} $1/usr/include ';'
if [ -d "bld" ]; then
    echo "Copying libc to $1/usr/lib"
    cp bld/libc.a $1/usr/lib/
fi

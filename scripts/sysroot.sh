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

#!/bin/bash

ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SystemRoot="$ScriptDirectory/../root"

run(){
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

echo -e "\n\n -> Bootstrapping System Root at $SystemRoot\n\n"
run mkdir -p "$SystemRoot"
# Copy base filesystem with pre-built libc
#     binaries into newly-created sysroot.
# This solves the issue of having to build a
#     bootstrap version of the compiler first.
cd $ScriptDirectory
run cp -r ../base/* "$SystemRoot/"
# Copy header files from libc to sysroot.
run cd ../user/libc/
run find ./ -name '*.h' -exec cp --parents '{}' -t $SystemRoot/inc ';'
run cd $ScriptDirectory

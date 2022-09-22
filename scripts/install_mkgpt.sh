#!/bin/bash
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


# Dependencies:
# |-- autoconf
# `-- automake

run(){
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

run sudo apt install autoconf automake
run git clone https://github.com/jncronin/mkgpt.git
run cd mkgpt
run automake --add-missing
run autoreconf
run ./configure
run make
run sudo make install

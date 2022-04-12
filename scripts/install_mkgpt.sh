#!/bin/bash
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

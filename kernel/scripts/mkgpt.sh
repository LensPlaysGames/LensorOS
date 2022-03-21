#!/bin/bash
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
KernelDirectory="$ScriptDirectory/.."

run(){
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

run $ScriptDirectory/mkimg.sh
run mkgpt -o $KernelDirectory/bin/LensorOS.bin --part $KernelDirectory/bin/LensorOS.img --type system

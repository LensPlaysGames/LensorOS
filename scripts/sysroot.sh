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

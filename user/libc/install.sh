#!/bin/bash
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

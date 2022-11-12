#!/usr/bin/env bash
set -eu

die() {
    echo "$1"
    exit 1
}

## Need at least one argument.
test $# -lt 2 || die "Usage: ./add-test.sh <path>"

project_path=$(dirname "$0")
test_path=$project_path/test
test_file=$test_path/$1.cpp
test_name=$1


## Make sure the file doesn’t already exist.
test -e "$test_file" && die "Error: File \"$1\" already exists"

## Make sure the parent directory of the file exists.
mkdir -p "$(dirname "$test_file")"

## Generate a test stub.
cat > "$test_file" << END
#include <testing.hh>

int main() {

}
END

## Add the test to the CMakeLists.txt.
echo -e "test(\"$test_name\" \"\${PROJECT_SOURCE_DIR}/test/$test_name.cpp\")" >> "$project_path/CMakeLists.txt"

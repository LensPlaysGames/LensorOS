cmake_minimum_required(VERSION 3.20)
project(tests C CXX ASM)
enable_testing()

# Don't build LibC for LensorOS.
set(LENSOR_OS_LIBC_BUILD_HOST 1)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(.. libc)

add_library(options INTERFACE)
target_compile_options(options INTERFACE
    -Wall -Wextra -Wconversion -Wundef
    -Werror=return-type
    -O0 -g
)
target_include_directories(options INTERFACE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(options INTERFACE c)

set(global_test_counter 0)

function(test name file)
    set(counter "${global_test_counter}")
    set(target_name "target${counter}")
    math(EXPR newcounter "${counter} + 1")
    set(global_test_counter "${newcounter}" PARENT_SCOPE)

    add_executable("${target_name}" "${file}")
    target_link_libraries("${target_name}" options)
    add_test("${name}" "${target_name}")
endfunction()

test("string/base" "${PROJECT_SOURCE_DIR}/test/string/base.cpp")
test("string/cat" "${PROJECT_SOURCE_DIR}/test/string/cat.cpp")
test("string/compare" "${PROJECT_SOURCE_DIR}/test/string/compare.cpp")
test("string/starts-ends" "${PROJECT_SOURCE_DIR}/test/string/starts-ends.cpp")
test("string/misc" "${PROJECT_SOURCE_DIR}/test/string/misc.cpp")
test("string/compare-lex" "${PROJECT_SOURCE_DIR}/test/string/compare-lex.cpp")
test("string/erase" "${PROJECT_SOURCE_DIR}/test/string/erase.cpp")
test("string/view" "${PROJECT_SOURCE_DIR}/test/string/view.cpp")
test("string/substr" "${PROJECT_SOURCE_DIR}/test/string/substr.cpp")

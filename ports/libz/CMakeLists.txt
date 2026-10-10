if (NOT DEFINED LENSOROS_LIB_Z OR NOT LENSOROS_LIB_Z)
  return()
endif()
function(msg MESSAGE)
  message(STATUS "[PORT]: library -- zlib: ${MESSAGE}")
endfunction()
msg("enabled")

include(FetchContent)

cmake_minimum_required(VERSION 3.21)
project(LensorOS_libz)

msg("disabling dependencies")
set(ZLIB_BUILD_TESTING OFF) # Disable Zlib Examples as tests
set(ZLIB_BUILD_SHARED OFF) # Disable building zlib shared library
set(ZLIB_BUILD_STATIC ON) # Enable building zlib static library
set(ZLIB_BUILD_MINIZIP OFF) # Disable building libminizip contrib library

msg("downloading source code")
FetchContent_Declare(
    libzsrc
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(libzsrc)

add_custom_target(libz)
add_dependencies(libz ZLIB::ZLIBSTATIC)

# TODO install zlib headers to sysroot /inc
# TODO install zlib library to sysroot /lib


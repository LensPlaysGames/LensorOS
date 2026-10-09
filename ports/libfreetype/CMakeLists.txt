if (NOT DEFINED LENSOROS_LIB_FREETYPE OR NOT LENSOROS_LIB_FREETYPE)
  return()
endif()
function(msg MESSAGE)
  message(STATUS "[PORT]: library -- freetype: ${MESSAGE}")
endfunction()
msg("enabled")

include(FetchContent)

cmake_minimum_required(VERSION 3.21)
project(LensorOS_libfreetype)

msg("disabling dependencies")
set(FT_DISABLE_ZLIB ON)
set(FT_DISABLE_BZIP2 ON)
set(FT_DISABLE_PNG ON)
set(FT_DISABLE_HARFBUZZ ON)
set(FT_DISABLE_BROTLI ON)
set(FT_DISABLE_HVF ON)

msg("downloading source code")
FetchContent_Declare(
    libfreetypesrc
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_TAG master
)
FetchContent_MakeAvailable(libfreetypesrc)

add_custom_target(libfreetype)
add_dependencies(libfreetype freetype)

# TODO install freetype headers to sysroot /inc
# TODO install freetype library to sysroot /lib

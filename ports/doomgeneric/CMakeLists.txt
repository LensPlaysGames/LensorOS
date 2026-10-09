if (NOT DEFINED LENSOROS_APP_DOOMGENERIC OR NOT LENSOROS_APP_DOOMGENERIC)
  return()
endif()
message(STATUS "[PORT]: DOOM (doomgeneric)")

include(FetchContent)

cmake_minimum_required(VERSION 3.14)
project(LensorOS_doomgeneric)

FetchContent_Declare(
    doomgenericsrc
    GIT_REPOSITORY https://github.com/LensPlaysGames/doomgeneric
    GIT_TAG main
)
set(
  PLATFORM_IMPLEMENTATION_FILE
  "doomgeneric/doomgeneric_lensor.c"
  CACHE STRING
  "The file that implements the non-portable doomgeneric functions"
)
FetchContent_MakeAvailable(doomgenericsrc)

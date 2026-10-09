if (NOT DEFINED LENSOROS_APP_DOOMGENERIC OR NOT LENSOROS_APP_DOOMGENERIC)
  return()
endif()
function(msg MESSAGE)
  message(STATUS "[PORT]:DOOM (doomgeneric): ${MESSAGE}")
endfunction()
msg("enabled")

include(FetchContent)

cmake_minimum_required(VERSION 3.21)
project(LensorOS_doomgeneric)

# doomgeneric is the engine, but it requires game assets to load.
# NOTE: please do not patch this with links to illegally obtained game assets ;)
#       *we only provide links to the publicly available "shareware" version.*
set(
  DOOM1_IWAD_WADARCHIVE
  "https://archive.org/download/wadarchive/DATA/5b.zip/5b%2F2e249b9c5133ec987b3ea77596381dc0d6bc1d%2F5b2e249b9c5133ec987b3ea77596381dc0d6bc1d.wad.gz"
)
set(
  DOOM1_IWAD_INTERNETARCHIVE
  "https://archive.org/download/doom-wads/Doom%20%28v1.9%29%20%28Demo%29.zip/DOOM1.WAD"
)
set(
  DOOM1_IWAD_PCFREAK # https://pc-freak.net/blog/doom-1-doom-2-doom-3-game-wad-files-for-download-playing-doom-on-debian-linux-via-freedoom-open-source-doom-engine/
  "https://www.pc-freak.net/files/doom-wad-files/Doom1.WAD" # requires renaming...
)
set(
  DOOM1_IWAD_LINK
  "${DOOM1_IWAD_INTERNETARCHIVE}"
  CACHE STRING
  "URI to download DOOM 1 v1.9 Shareware WAD (game asset file)"
)
set(
  DOOM1_IWAD_PATH
  "${CMAKE_CURRENT_LIST_DIR}/DOOM1.WAD"
  CACHE PATH
  "File path to find DOOM1.WAD"
)

if (EXISTS "${DOOM1_IWAD_PATH}")
  msg("using existing game assets at '${DOOM1_IWAD_PATH}'")
else()
  msg("downloading game assets from '${DOOM1_IWAD_LINK}'")
  file(
    DOWNLOAD
    "${DOOM1_IWAD_LINK}"
    "${DOOM1_IWAD_PATH}"
    SHOW_PROGRESS
    INACTIVITY_TIMEOUT 60
  )
  if (NOT EXISTS "${DOOM1_IWAD_PATH}")
    message(FATAL_ERROR "could not fetch DOOM1.WAD")
  endif()
endif()

msg("installing game assets")
file(
  COPY_FILE
  "${DOOM1_IWAD_PATH}"
  "${PORT_OUTPUT_DIRECTORY}/DOOM1.WAD"
)

msg("downloading engine source")
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

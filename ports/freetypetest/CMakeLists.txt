if (NOT DEFINED LENSOROS_LIB_FREETYPE OR NOT LENSOROS_LIB_FREETYPE)
  return()
endif()
function(msg MESSAGE)
  message(STATUS "[PORT]: application -- freetypetest: ${MESSAGE}")
endfunction()
msg("enabled")

cmake_minimum_required(VERSION 3.14)
project(LensorOS_freetypetest LANGUAGES CXX)

add_executable(
  freetypetest
  main.cpp
)

target_link_libraries(freetypetest PUBLIC freetype)

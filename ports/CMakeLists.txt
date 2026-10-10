cmake_minimum_required( VERSION 3.20 )
project( LensorOSPorts VERSION ${LensorOS_VERSION} LANGUAGES ${LensorOS_LANGUAGES} )

include(config.cmake)

set(
  PORT_OUTPUT_DIRECTORY
  ${CMAKE_CURRENT_LIST_DIR}/../user/bin
  CACHE PATH
  "This is where built libraries and programs will be installed."
)

add_custom_target( ported-libraries )
add_custom_target( ported-programs )
add_dependencies( ported-programs ported-libraries )

add_dependencies( ported-libraries userspace-libraries )
add_dependencies( userspace ported-programs ported-libraries )

function(add_ported_library NAME )
  add_subdirectory( ${NAME} EXCLUDE_FROM_ALL )
  if (TARGET ${NAME})
    set_property(
      TARGET ${NAME}
      PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PORT_OUTPUT_DIRECTORY}
    )
    add_dependencies(ported-libraries ${NAME})
  endif()
endfunction()

function(add_ported_program NAME )
  add_subdirectory( ${NAME} EXCLUDE_FROM_ALL )
  if (TARGET ${NAME})
    set_property(
      TARGET ${NAME}
      PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PORT_OUTPUT_DIRECTORY}
    )
    add_dependencies(${NAME} ported-libraries)
    add_dependencies(ported-programs ${NAME})
  endif()
endfunction()

add_ported_library(libz)
add_ported_library(libfreetype)

add_ported_program(doomgeneric)
add_ported_program(freetypetest)

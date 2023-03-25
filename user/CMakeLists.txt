cmake_minimum_required( VERSION 3.20 )
project( LensorOS VERSION ${LensorOS_VERSION} LANGUAGES ${LensorOS_LANGUAGES} )

set( USERSPACE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin CACHE PATH "This is where built userspace libraries and programs will be installed." )

add_custom_target( userspace-libraries )
add_custom_target( userspace )


# LIBRARIES

# NOTE: Each library passed to this function *must* have a subdirectory
# in `user` with a valid CMakeLists.txt within it prefixed with "lib",
# as well as only generate one target that is named the same as the
# subdirectory.
function(add_userspace_library NAME)
  add_subdirectory( lib${NAME} EXCLUDE_FROM_ALL )
  set_property(
    TARGET ${NAME}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${USERSPACE_OUTPUT_DIRECTORY}
  )
  add_dependencies( userspace ${NAME} )
endfunction()

add_userspace_library( c )
add_dependencies( userspace-libraries install-libc )
add_userspace_library( m )
add_dependencies( userspace-libraries install-libm )
add_userspace_library( stdc++ )
add_dependencies( userspace-libraries install-libstdc++ )

add_dependencies( userspace userspace-libraries )

# PROGRAMS

# NOTE: Each program passed to this function *must* have a subdirectory
# in `user` with a valid CMakeLists.txt within it, as well as only
# generate one target that is named the same as the subdirectory.
function(add_userspace_program NAME )
  add_subdirectory( ${NAME} EXCLUDE_FROM_ALL )
  add_dependencies( ${NAME} install-libc ) # depend on libc by default
  set_property(
    TARGET ${NAME}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${USERSPACE_OUTPUT_DIRECTORY}
  )
  add_dependencies( userspace ${NAME} )
endfunction()

function(add_cxx_userspace_program NAME )
  add_userspace_program( ${NAME} )
  # g++ automatically links with these libraries
  add_dependencies( ${NAME} install-libm )
  add_dependencies( ${NAME} install-libstdc++ )
endfunction()


add_userspace_program( blazeit )
add_userspace_program( pwd )
add_userspace_program( stdout )
add_cxx_userspace_program( xish )
add_cxx_userspace_program( echo )


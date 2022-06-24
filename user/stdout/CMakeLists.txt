cmake_minimum_required( VERSION 3.14 )
set( stdout_VERSION 0.0.1 )
set( stdout_LANGUAGES C )

# Export compilation database in JSON format.
set( CMAKE_EXPORT_COMPILE_COMMANDS on )

# Use LensorOS GNU-based toolchain by default.
set( CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../kernel/lensor_gnu_toolchain.cmake" )

project( stdout VERSION ${stdout_VERSION} LANGUAGES ${stdout_LANGUAGES} )

add_executable( stdout main.c )

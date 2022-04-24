cmake_minimum_required( VERSION 3.20 )
set( blazeit_VERSION 0.0.1 )
set( blazeit_LANGUAGES C )

# Get the directory of the repository.
cmake_path(
  GET
  CMAKE_CURRENT_LIST_DIR
  PARENT_PATH
  REPO_DIR
)
cmake_path(
  GET
  REPO_DIR
  PARENT_PATH
  REPO_DIR
)

# Use LensorOS GNU-based toolchain by default.
set( CMAKE_TOOLCHAIN_FILE "${REPO_DIR}/kernel/lensor_gnu_toolchain.cmake" )

project( LensorOS VERSION ${blazeit_VERSION} LANGUAGES ${blazeit_LANGUAGES} )

add_executable( blazeit main.c )

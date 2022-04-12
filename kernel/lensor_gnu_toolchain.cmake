include( "${CMAKE_CURRENT_LIST_DIR}/config.cmake" )

# Until a LensorOS config is added to the CMake source code,
# then subsequently released, `Generic` is used to avoid errors.
set( CMAKE_SYSTEM_NAME Generic )
set( CMAKE_SYSTEM_VERSION ${LensorOS_VERSION} )

# Don't try to link during `try_compile()`.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set sysroot
set( CMAKE_SYSROOT "${CMAKE_CURRENT_LIST_DIR}/../root" )
if( NOT EXISTS "${CMAKE_SYSROOT}" )
  message(
	FATAL_ERROR
	"The sysroot at ${CMAKE_SYSROOT} does not exist"
  )
endif()

# Don't look for host programs in the target sysroot.
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
# Do look for libraries and includes in the target sysroot.
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

# Use the LensorOS GNU Toolchain for C and C++.
set( CMAKE_C_COMPILER ${ARCH}-lensor-gcc )
set( CMAKE_CXX_COMPILER ${ARCH}-lensor-g++ )

# If the toolchain isn't in PATH, try to find it manually.
if( NOT COMMAND "${CMAKE_CXX_COMPILER}" )
  message( VERBOSE "${CMAKE_CXX_COMPILER} not found in PATH" )
  set( TOOLCHAIN_DIR "${CMAKE_CURRENT_LIST_DIR}/../toolchain" )
  if( CMAKE_HOST_WIN32 )
	set( TOOLCHAIN_BIN "${TOOLCHAIN_DIR}/wincross/bin" )
	message(
	  CHECK_START
	  "Looking for ${CMAKE_CXX_COMPILER}.exe in ${TOOLCHAIN_BIN}"
	)
	if( EXISTS "${TOOLCHAIN_BIN}/${CMAKE_CXX_COMPILER}.exe" )
	  message( CHECK_PASS "found" )
	  set( ENV{PATH} "${TOOLCHAIN_BIN};$ENV{PATH}" )
	else()
	  message( CHECK_FAIL "not found")
	  message(
		FATAL_ERROR
		"${CMAKE_CXX_COMPILER} not found in PATH or ${TOOLCHAIN_BIN}"
	  )
	endif()
  else()
	set( TOOLCHAIN_BIN "${TOOLCHAIN_DIR}/cross/bin" )
	message(
	  CHECK_START
	  "Looking for ${CMAKE_CXX_COMPILER} in ${TOOLCHAIN_BIN}"
	)
	if( EXISTS "${TOOLCHAIN_BIN}/${CMAKE_CXX_COMPILER}" )
	  message( CHECK_PASS "found" )
	  set( ENV{PATH} "${TOOLCHAIN_BIN}:$ENV{PATH}" )
	else()
	  message( CHECK_PASS "not found" )
	  message(
		FATAL_ERROR
		"${CMAKE_CXX_COMPILER} not found in PATH or ${TOOLCHAIN_BIN}"
	  )
	endif()
  endif()
endif()

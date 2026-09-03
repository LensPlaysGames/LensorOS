set(
  LENSOR_LINK_FLAGS_LIST
  "-target ${TARGET_TRIPLE}"
  "-fuse-ld=lld"
  "-nostdlib"
  "-no-pie"
)

set(
  LENSOR_C_FLAGS_LIST
  "-target ${TARGET_TRIPLE}"
  "-nodefaultlibs"
  "-nostdlib"
  "-fno-pie"
)

set(
  LENSOR_CXX_FLAGS_LIST
  "-fno-stack-protector"
  "-fno-exceptions"
  "-fno-rtti"
)

if (NOT LENSOR_OS_LIBC_BUILD_HOST)
  add_compile_definitions(
    __lensor__
    __lensoros__
    __unix__
  )
endif()

# Userspace Only Configuration (Standard Libraries and stuff)
if (LENSOROS_KERNEL_BUILD)
  list( APPEND LENSOR_C_FLAGS_LIST "-ffreestanding")
  set( CMAKE_C_STANDARD_LIBRARIES "")
  set( CMAKE_CXX_STANDARD_LIBRARIES "" )
  set( CMAKE_LINK_DIRECTORIES "" )
  set(
    CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
  )
  set(
    CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
  )
else()
  set(
    CMAKE_C_STANDARD_LIBRARIES
    "${CMAKE_SYSROOT}/lib/libc.a"
  )
  set(
    CMAKE_CXX_STANDARD_LIBRARIES
    "${CMAKE_SYSROOT}/lib/libstdc++.a ${CMAKE_SYSROOT}/lib/libc.a"
  )
  set(
    CMAKE_LINK_DIRECTORIES
    "${CMAKE_SYSROOT}/lib"
  )

  list(
    APPEND LENSOR_LINK_FLAGS_LIST
    "-B${CMAKE_SYSROOT}/lib/"
  )

  set(
    CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> ${CMAKE_SYSROOT}/lib/crt0.o ${CMAKE_SYSROOT}/lib/crti.o <OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${CMAKE_SYSROOT}/lib/crtn.o"
  )
  set(
    CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> ${CMAKE_SYSROOT}/lib/crt0.o ${CMAKE_SYSROOT}/lib/crti.o <OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${CMAKE_SYSROOT}/lib/crtn.o"
  )
endif()

# ================================================================
# Apply Flags
# ================================================================
string( JOIN " " LENSOR_LINK_FLAGS ${LENSOR_LINK_FLAGS_LIST} )
set(CMAKE_EXE_LINKER_FLAGS "${LENSOR_LINK_FLAGS}")

string( JOIN " " LENSOR_C_FLAGS ${LENSOR_C_FLAGS_LIST} )
set(CMAKE_C_FLAGS "${LENSOR_C_FLAGS}")

string( JOIN " " LENSOR_CXX_FLAGS ${LENSOR_CXX_FLAGS_LIST} )
set(CMAKE_CXX_FLAGS "${LENSOR_C_FLAGS} ${LENSOR_CXX_FLAGS}")

# NOTE: Relies on user/CMakeLists.txt to define custom rules that link
# crt*.o at the right points.

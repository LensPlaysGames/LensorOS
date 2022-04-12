cmake_minimum_required( VERSION 3.14 )
set( LensorOS_VERSION 0.0.1 )
set( LensorOS_LANGUAGES ASM_NASM CXX )

include( "${CMAKE_CURRENT_LIST_DIR}/config.cmake" )

# Use LensorOS GNU-based toolchain by default.
set( CMAKE_TOOLCHAIN_FILE lensor_gnu_toolchain.cmake )

# Use `ccache` if it is installed in system's PATH.
find_program( CCACHE_PROGRAM ccache )
if ( CCACHE_PROGRAM )
  set_property( GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}" )
endif ()

# Export compilation database in JSON format.
set( CMAKE_EXPORT_COMPILE_COMMANDS on )

# Output executable files to `/bin` directory.
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin )


project( LensorOS VERSION ${LensorOS_VERSION} LANGUAGES ${LensorOS_LANGUAGES} )

# Interrupts must be compiled with general registers only.
add_library(
  Interrupts
  src/interrupts/interrupts.cpp
  src/interrupts/interrupts.h
)
target_compile_options(
  Interrupts PRIVATE
  -O3
  -ffreestanding
  -fno-exceptions
  -mcmodel=kernel
  -mgeneral-regs-only
  -mno-red-zone
  -c
)
target_include_directories( Interrupts PRIVATE src/ )

add_library(
  Assembly
  src/cpuid.asm
  src/gdt.asm
  src/interrupts/syscalls.asm
  src/prekernel.asm
  src/scheduler.asm
  src/userswitch.asm
)
string( APPEND CMAKE_ASM_NASM_COMPILE_OBJECT " -f elf64" )


set(
  KERNEL_SOURCES
  src/acpi.cpp
  src/ahci.cpp
  src/basic_renderer.cpp
  src/bitmap.cpp
  src/cpuid.cpp
  src/cstr.cpp
  src/efi_memory.cpp
  src/gdt.cpp
  src/gpt.cpp
  src/hpet.cpp
  src/interrupts/idt.cpp
  src/interrupts/syscalls.cpp
  src/io.cpp
  src/kernel.cpp
  src/keyboard.cpp
  src/keyboard_scancode_translation.cpp
  src/kstage1.cpp
  src/memory.cpp
  src/memory/heap.cpp
  src/memory/physical_memory_manager.cpp
  src/memory/virtual_memory_manager.cpp
  src/mouse.cpp
  src/panic.cpp
  src/pci.cpp
  src/pci_descriptors.cpp
  src/pit.cpp
  src/pure_virtuals.cpp
  src/random_lcg.cpp
  src/random_lfsr.cpp
  src/rtc.cpp
  src/scheduler.cpp
  src/spinlock.cpp
  src/system.cpp
  src/tss.cpp
  src/uart.cpp
)

add_executable( Kernel ${KERNEL_SOURCES} )
set_target_properties( Kernel PROPERTIES OUTPUT_NAME kernel.elf )
target_compile_definitions( Kernel PUBLIC ${MACHINE} ${ARCH} )
target_compile_options(
  Kernel PUBLIC
  -ffreestanding
  -fno-exceptions
  -fno-rtti
  -fshort-wchar
  -mcmodel=kernel
  -mno-red-zone
  -O3
  -Wall
  -Wextra
  -Werror
)
target_link_options(
  Kernel PUBLIC
  -T ${CMAKE_CURRENT_LIST_DIR}/kernel.ld
  -static
  -Bsymbolic
  -nostdlib
  -z max-page-size=0x1000
)
target_include_directories( Kernel PUBLIC src/ )
target_link_libraries( Kernel PUBLIC Interrupts )
target_link_libraries( Kernel PUBLIC Assembly )

# Ensure `/bin` directory exists.
add_custom_target(
  EnsureBinDirectory ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory
  ${CMAKE_SOURCE_DIR}/bin
)
# Copy contents of the resources directory into the final binary directory.
add_custom_command(
  TARGET EnsureBinDirectory POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/res
  ${CMAKE_SOURCE_DIR}/bin
)

# When using a Canadian Cross LensorOS Toolchain, the sysroot
# is often different than what it is during the toolchain build.
# This means it must be explicitly specified on the command line.
if ( CMAKE_HOST_WIN32 )
  target_compile_options(
    Interrupts PRIVATE
    --sysroot=${CMAKE_SOURCE_DIR}\\..\\root
  )
  target_compile_options(
    Kernel PUBLIC
    --sysroot=${CMAKE_SOURCE_DIR}\\..\\root
  )
endif ()

cmake_minimum_required(VERSION 3.14)
set(LensorOS_VERSION 0.0.1)
set(LensorOS_LANGUAGES ASM_NASM CXX)

# MACHINE can be one of PC, QEMU, VBOX.
set(MACHINE VBOX)
# ARCH can be x86_64 only, for now.
set(ARCH x86_64)

# Until a LensorOS config is added to the CMake source code,
# then subsequently released, `Generic` is used to avoid errors.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION ${LensorOS_VERSION})

# Use the LensorOS GNU Toolchain for C++.
set(CMAKE_CXX_COMPILER ${ARCH}-lensor-g++)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Use `ccache` if it is installed in system's PATH.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Export compilation database in JSON format.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# Output executable files to `/bin` directory.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)


project(LensorOS VERSION ${LensorOS_VERSION} LANGUAGES ${LensorOS_LANGUAGES})

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

add_library(
  Assembly
  src/cpuid.asm
  src/gdt.asm
  src/interrupts/syscalls.asm
  src/prekernel.asm
  src/scheduler.asm
  src/userswitch.asm
  )
string(APPEND CMAKE_ASM_NASM_COMPILE_OBJECT " -f elf64")

set(
  KERNEL_HEADERS
  src/acpi.h
  src/ahci.h
  src/basic_renderer.h
  src/bitmap.h
  src/boot.h
  src/cpu.h
  src/cpuid.h
  src/cstr.h
  src/efi_memory.h
  src/fat_driver.h
  src/fat_definitions.h
  src/fat_fs.h
  src/filesystem.h
  src/gdt.h
  src/gpt.h
  src/guid.h
  src/hpet.h
  src/integers.h
  src/interrupts/idt.h
  src/interrupts/syscalls.h
  src/inode.h
  src/io.h
  src/keyboard.h
  src/keyboard_scancode_translation.h
  src/kstage1.h
  src/large_integers.h
  src/link_definitions.h
  src/linked_list.h
  src/math.h
  src/memory.h
  src/memory/heap.h
  src/memory/paging.h
  src/memory/virtual_memory_manager.h
  src/mouse.h
  src/panic.h
  src/pci.h
  src/pit.h
  src/pure_virtuals.h
  src/random_lcg.h
  src/random_lfsr.h
  src/memory/region.h
  src/rtc.h
  src/scheduler.h
  src/smart_pointer.h
  src/spinlock.h
  src/system.h
  src/tss.h
  src/uart.h
  )

set(
  KERNEL_SOURCES
  src/acpi.cpp
  src/ahci.cpp
  src/basic_renderer.cpp
  src/bitmap.cpp
  src/cpuid.cpp
  src/cstr.cpp
  src/efi_memory.cpp
  src/fat_driver.cpp
  src/filesystem.cpp
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
  src/tss.cpp
  src/uart.cpp
  )

add_executable(Kernel ${KERNEL_SOURCES} ${KERNEL_HEADERS})
set_target_properties(Kernel PROPERTIES OUTPUT_NAME kernel.elf)
target_compile_definitions(Kernel PUBLIC ${MACHINE} ${ARCH})
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
  -T ${CMAKE_SOURCE_DIR}/scripts/kernel.ld
  -static
  -Bsymbolic
  -nostdlib
  )

# When using a Canadian Cross LensorOS Toolchain, the sysroot
# is often different than what it is during the build.
if(CMAKE_HOST_WIN32)
  target_compile_options(
    Interrupts PRIVATE
    --sysroot=${CMAKE_SOURCE_DIR}\\..\\root
    )
  target_compile_options(
    Kernel PUBLIC
    --sysroot=${CMAKE_SOURCE_DIR}\\..\\root
    )
endif()

target_link_libraries(Kernel PUBLIC Interrupts)
target_link_libraries(Kernel PUBLIC Assembly)

# Ensure `/bin` directory exists.
add_custom_target(EnsureBinDirectory ALL COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin)
# Copy contents of the resources directory into the final binary directory.
add_custom_command(TARGET EnsureBinDirectory POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/res
  ${CMAKE_SOURCE_DIR}/bin
  )

# Copy the generated `compile_commands.json` to the project
#   root directory in order for language servers like `clangd`
#   or `ccls` to detect it automatically even in out-of-source builds.
add_custom_target(
  CopyCompileCommands ALL
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_BINARY_DIR}/compile_commands.json
  ${CMAKE_SOURCE_DIR}/../compile_commands.json
  )

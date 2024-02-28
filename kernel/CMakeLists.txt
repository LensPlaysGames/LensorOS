# Copyright 2022, Contributors To LensorOS.
# All rights reserved.
#
# This file is part of LensorOS.
#
# LensorOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LensorOS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LensorOS. If not, see <https://www.gnu.org/licenses

# Export compilation database in JSON format.
set( CMAKE_EXPORT_COMPILE_COMMANDS on )

# Output executable files to `/bin` directory.
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin )


add_definitions(-D__kernel__)

# Interrupts must be compiled with general registers only.
add_library(
  Interrupts
  src/interrupts/interrupts.cpp
  src/panic.cpp
)
target_compile_options(
  Interrupts PRIVATE
  -ffreestanding
  -fshort-wchar

  -fno-exceptions
  -fno-tree-vectorize
  -fno-fast-math
  -fno-associative-math

  -mcmodel=kernel
  -mgeneral-regs-only
  -mno-red-zone
  -mno-sse
  -mno-sse2

  -c
)
target_include_directories( Interrupts PRIVATE src/ ${CMAKE_CURRENT_LIST_DIR}/../std/include )

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

add_executable(
  Kernel
  src/acpi.cpp
  src/ahci.cpp
  src/basic_renderer.cpp
  src/bitmap.cpp
  src/cpuid.cpp
  src/cstr.cpp
  src/debug.cpp
  src/devices/devices.cpp
  src/e1000.cpp
  src/efi_memory.cpp
  src/event.cpp
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
  src/pci.cpp
  src/pci_descriptors.cpp
  src/pit.cpp
  src/pure_virtuals.cpp
  src/random_lcg.cpp
  src/random_lfsr.cpp
  src/rtc.cpp
  src/scheduler.cpp
  src/spinlock.cpp
  src/storage/filesystem_drivers/input.cpp
  src/storage/filesystem_drivers/pipe.cpp
  src/storage/device_drivers/port_controller.cpp
  src/storage/device_drivers/socket.cpp
  src/storage/filesystem_drivers/file_allocation_table.cpp
  src/system.cpp
  src/tss.cpp
  src/uart.cpp
  src/virtual_filesystem.cpp
)
set_target_properties( Kernel PROPERTIES OUTPUT_NAME kernel.elf )
target_compile_definitions(
  Kernel PRIVATE
  ${MACHINE} ${ARCH}
)
if( HIDE_UART_COLOR_CODES )
  target_compile_definitions(
    Kernel PRIVATE
    "LENSOR_OS_UART_HIDE_COLOR_CODES"
  )
endif()
target_compile_options(
  Kernel PRIVATE
  -ffreestanding
  -fshort-wchar

  -fno-use-cxa-atexit
  -fno-exceptions
  -fno-rtti
  -fno-tree-vectorize
  -fno-fast-math
  -fno-associative-math

  -mcmodel=kernel

  -mno-red-zone
  -mno-sse
  -mno-sse2

  -Wall
  -Wextra
  -Werror

  -Wno-unused-parameter

  ## C++20 deprecates volatile |= and &=. That's nonsense, so we disable this warning.
  ##
  ## If you don't believe me that it's nonsense, C++23 de-deprecates them because they
  ## realised that this is nonsense.
  -Wno-volatile
)
target_link_options(
  Kernel PRIVATE
  -T ${CMAKE_CURRENT_LIST_DIR}/kernel.ld
  -static
  -Bsymbolic
  -nostdlib
  -z max-page-size=0x1000
)
target_include_directories( Kernel PRIVATE src/ ${CMAKE_CURRENT_LIST_DIR}/../std/include )
target_link_libraries( Kernel PRIVATE Interrupts )
target_link_libraries( Kernel PRIVATE Assembly )

# Ensure `kernel/bin` directory exists and copy `kernel/res` into it.
add_custom_command(
  TARGET Kernel
  PRE_BUILD
  COMMAND
  ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/boot/LensorOS
  COMMAND
  ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_LIST_DIR}/res ${PROJECT_SOURCE_DIR}/boot/LensorOS
  VERBATIM
)

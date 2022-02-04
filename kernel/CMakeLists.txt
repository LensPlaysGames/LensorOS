cmake_minimum_required(VERSION 3.14)
project(LensorOS VERSION 0.0.1 LANGUAGES ASM_NASM C CXX)

# Output files to `kernel/bin` directory
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

add_library(Interrupts src/interrupts/interrupts.cpp src/interrupts/interrupts.h)
target_compile_options(Interrupts PRIVATE -mno-red-zone -mgeneral-regs-only -ffreestanding -fno-exceptions -c)

add_library(Assembly src/gdt.asm)
target_compile_options(Assembly PRIVATE -felf64)

set(SOURCES
	src/acpi.cpp
	src/ahci.cpp
	src/basic_renderer.cpp
	src/bitmap.cpp
	src/cstr.cpp
	src/efi_memory.cpp
	src/FATDriver.cpp
	src/filesystem.cpp
	src/gdt.cpp
	src/heap.cpp
	src/interrupts/idt.cpp
	src/io.cpp
	src/kernel.cpp
	src/keyboard.cpp
	src/keyboard_scancode_translation.cpp
	src/kUtility.cpp
	src/memory.cpp
	src/mouse.cpp
	src/paging/page_frame_allocator.cpp
	src/paging/page_table_manager.cpp
	src/paging/paging.cpp
	src/panic.cpp
	src/pci.cpp
	src/pci_descriptors.cpp
	src/pit.cpp
	src/pure_virtuals.cpp
	src/rtc.cpp
	src/uart.cpp
)

set(HEADERS
	src/acpi.h
	src/ahci.h
	src/basic_renderer.h
	src/bitmap.h
	src/cstr.h
	src/efi_memory.h
	src/FAT_definitions.h
	src/FatFS.h
	src/filesystem.h
	src/gdt.h
	src/heap.h
	src/integers.h
	src/interrupts/idt.h
	src/io.h
	src/keyboard.h
	src/keyboard_scancode_translation.h
	src/kUtility.h
	src/large_integers.h
	src/math.h
	src/memory.h
	src/mouse.h
	src/paging/page_frame_allocator.h
	src/paging/page_table_manager.h
	src/paging/paging.h
	src/panic.h
	src/pci.h
	src/pit.h
	src/pure_virtuals.h
	src/rtc.h
	src/uart.h
	src/vfs_inode.h
)

add_executable(Kernel ${SOURCES} ${HEADERS})
target_compile_definitions(Kernel PUBLIC -DQEMU)

target_compile_options(Kernel PUBLIC -ffreestanding -fno-rtti -fshort-wchar -mno-red-zone -fno-exceptions -Wall -Wextra -Werror)

target_link_options(Kernel
	PUBLIC -T ${CMAKE_SOURCE_DIR}/kernel.ld -static -Bsymbolic -nostdlib
)							

target_link_libraries(Kernel PUBLIC Interrupts)
target_link_libraries(Kernel PUBLIC Assembly)
set_target_properties(Kernel PROPERTIES OUTPUT_NAME kernel.elf)

add_custom_target(EnsureBinDirectory ALL COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin)
add_custom_command(TARGET EnsureBinDirectory POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res
		${CMAKE_SOURCE_DIR}/bin)
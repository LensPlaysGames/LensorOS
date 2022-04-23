cmake_minimum_required( VERSION 3.20 )
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
  src/debug.cpp
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

# Ensure `kernel/bin` directory exists and copy `kernel/res` into it.
add_custom_command(
  TARGET Kernel
  PRE_BUILD
  COMMAND
  ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_LIST_DIR}/bin
  COMMAND
  ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_LIST_DIR}/res ${CMAKE_CURRENT_LIST_DIR}/bin
  VERBATIM
)

# Get the directory of the repository.
cmake_path(
  GET
  CMAKE_CURRENT_LIST_DIR
  PARENT_PATH
  REPO_DIR
)
set( SCRIPTS_DIR "${REPO_DIR}/scripts")
cmake_path(
  NATIVE_PATH
  SCRIPTS_DIR
  NORMALIZE
  SCRIPTS_DIR
)
set( IMAGE_DIR "${REPO_DIR}/kernel/bin" )
cmake_path(
  NATIVE_PATH
  IMAGE_DIR
  NORMALIZE
  IMAGE_DIR
)

# Boot media generation targets
find_program( DD_PROGRAM dd )
find_program( MTOOLS_PROGRAM mtools )
if( CMAKE_HOST_UNIX )
  find_program( BASH_PROGRAM bash )
  # Boot media generation: raw FAT32 image.
  if( BASH_PROGRAM AND DD_PROGRAM AND MTOOLS_PROGRAM )
    message(
      VERBOSE
      "Found bash, dd, and mtools, creating target: image_raw"
    )
    add_custom_target(
      image_raw
      COMMAND ${BASH_PROGRAM} mkimg.sh
      WORKING_DIRECTORY ${SCRIPTS_DIR}
      DEPENDS
      ${REPO_DIR}/gnu-efi/x86_64/bootloader/main.efi
      ${SCRIPTS_DIR}/startup.nsh
      COMMENT "Generating UEFI-compatible FAT32 boot media"
      USES_TERMINAL
      VERBATIM
    )
    add_dependencies( image_raw Kernel )
  endif()
endif()

if( CMAKE_HOST_WIN32 )
  # Boot media generation: raw FAT32 image.
  if( DD_PROGRAM AND MTOOLS_PROGRAM )
    message(
      VERBOSE
      "Found dd and mtools, creating target: image_raw"
    )
    add_custom_target(
      image_raw
      COMMAND cmd /c ${SCRIPTS_DIR}/mkimg.bat
      COMMENT "Generating UEFI-compatible FAT32 boot media"
      DEPENDS
      ${REPO_DIR}/gnu-efi/x86_64/bootloader/main.efi
      ${SCRIPTS_DIR}/startup.nsh
      USES_TERMINAL
      VERBATIM
    )
    # Image generation relies on kernel.elf
    add_dependencies( image_raw Kernel )
  endif()
endif()

# Boot media generation: GPT partitioned hard drive.
find_program( CREATEGPT_PROGRAM createGPT )
if( CREATEGPT_PROGRAM )
  message(
    VERBOSE
    "Found createGPT, creating target: image_gpt"
  )
  add_custom_target(
    image_gpt
    COMMAND
    ${CREATEGPT_PROGRAM}
    -o ${IMAGE_DIR}/LensorOS.bin
    -p ${IMAGE_DIR}/LensorOS.img --type system
    COMMENT "Generating GUID Partion Table with EFI System partion containing boot media using `createGPT`"
    USES_TERMINAL
    VERBATIM
  )
elseif( CMAKE_HOST_UNIX )
  find_program( FDISK_PROGRAM fdisk )
  if( BASH_PROGRAM AND FDISK_PROGRAM )
    message(
      VERBOSE
      "Found bash and fdisk, creating target: image_gpt"
    )
    add_custom_target(
      image_gpt
      COMMAND ${BASH_PROGRAM} mkgpt_fdisk.sh
      WORKING_DIRECTORY ${SCRIPTS_DIR}
      COMMENT "Generating GUID Partion Table with EFI System partion containing boot media using `fdisk`"
      USES_TERMINAL
      VERBATIM
    )
  else()
    find_program( MKGPT_PROGRAM mkgpt )
    if( MKGPT_PROGRAM )
      message(
        VERBOSE
        "Found mkgpt, creating target: image_gpt"
      )
      add_custom_target(
        image_gpt
        COMMAND ${MKGPT_PROGRAM}
        -o ${IMAGE_DIR}/LensorOS.bin
        --part ${IMAGE_DIR}/LensorOS.img --type system
        COMMENT "Generating GUID Partion Table with EFI System partion containing boot media using `mkgpt`"
        USES_TERMINAL
        VERBATIM
      )
    endif()
  endif()
endif()

# Boot media generation: ISO-9660 CD-ROM.
find_program( XORRISO_PROGRAM xorriso )
if( XORRISO_PROGRAM )
  message(
    VERBOSE
    "Found xorriso, creating target: image_iso"
  )
  add_custom_target(
    image_iso
    COMMAND ${CMAKE_COMMAND} -E make_directory iso
    COMMAND ${CMAKE_COMMAND} -E copy LensorOS.img iso
    COMMAND
    ${XORRISO_PROGRAM} -as mkisofs
    -R -f
    -e LensorOS.img
    -no-emul-boot
    -o LensorOS.iso iso
    COMMAND rm -R iso
    COMMENT "Generating El-Torito ISO-9660 boot media"
    WORKING_DIRECTORY ${IMAGE_DIR}
    USES_TERMINAL
    VERBATIM
  )
else()
  message(
    "-- xorriso not found on system, skipping image_iso target generation."
  )
endif()

if( TARGET image_raw )
  if( TARGET image_gpt )
    add_dependencies( image_gpt image_raw )
  endif()
  if( TARGET image_iso )
    add_dependencies( image_iso image_raw )
  endif()
endif()


# Add a custom target to run QEMU with the proper flags
# if QEMU for x86_64 is found in the PATH environment variable.
find_program( QEMU_PROGRAM qemu-system-x86_64 )
if ( QEMU_PROGRAM )
  message(
    VERBOSE
    "Found QEMU for x86_64, creating targets: runimg_qemu runhda_qemu runiso_qemu"
  )
  set(
    QEMU_FLAGS
    -machine q35
    # cpu: Broadwell, Cascadelake-Server, Cooperlake, Conroe, core2duo,
    #      Denverton, Dhyana, EPYC, Haswell, IvyBridge, kvm64, max,
    #      Nehalem, Penryn, qemu64, SandyBridge, Skylake-[Client|Server],
    #      Snowridge, Westmere
    -cpu qemu64
    # Multiple processors (commented until utilized).
    #-smp 8,sockets=2,cores=2,threads=2,maxcpus=8
    # One hundred megabytes of RAM.
    -m 100M
    # Use local time as real time clock base.
    -rtc base=localtime,clock=host,driftfix=none
    # This is deprecated, but we'll use it,
    # until we write an actual sound driver.
    -soundhw pcspk
    # No networking drivers yet.
    -net none
    # Show extra information regarding triple faults.
    -d cpu_reset
    # Use stdio as serial input and output.
    # This allows debug messages to reach the terminal.
    -serial stdio
  )
  set( OVMF_CODE "${REPO_DIR}/OVMFbin/OVMF_CODE-pure-efi.fd" )
  set( OVMF_VARS "${REPO_DIR}/OVMFbin/OVMF_VARS_LensorOS.fd" )
  cmake_path(
    NATIVE_PATH
    OVMF_CODE
    NORMALIZE
    OVMF_CODE
  )
  cmake_path(
    NATIVE_PATH
    OVMF_VARS
    NORMALIZE
    OVMF_VARS
  )
  list(
    APPEND QEMU_FLAGS
    -drive if=pflash,format=raw,unit=0,file=${OVMF_CODE},readonly=on
    -drive if=pflash,format=raw,unit=1,file=${OVMF_VARS}
  )
  add_custom_target(
    runimg_qemu
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -drive format=raw,file=${IMAGE_DIR}/LensorOS.img
    COMMENT "Running QEMU for x86_64 from FAT32 `LensorOS.img`"
    USES_TERMINAL
    WORKING_DIRECTORY ${REPO_DIR}
    VERBATIM
  )
  add_custom_target(
    runhda_qemu
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -drive format=raw,file=${IMAGE_DIR}/LensorOS.bin
    COMMENT "Running QEMU for x86_64 from GPT `LensorOS.bin`"
    USES_TERMINAL
    WORKING_DIRECTORY ${REPO_DIR}
    VERBATIM
  )
  add_custom_target(
    runiso_qemu
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -drive format=raw,file=${IMAGE_DIR}/LensorOS.iso,media=cdrom
    COMMENT "Running QEMU for x86_64 from ISO `LensorOS.iso`"
    USES_TERMINAL
    WORKING_DIRECTORY ${REPO_DIR}
    VERBATIM
  )
  # Dependencies
  if( TARGET image_raw )
    add_dependencies( runimg_qemu image_raw )
  endif()
  if( TARGET image_gpt )
    add_dependencies( runhda_qemu image_gpt )
  endif()
  if( TARGET image_iso )
    add_dependencies( runiso_qemu image_iso )
  endif()
endif()

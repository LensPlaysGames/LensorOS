# HERES THE IDEA
# cmake --build bld will generate GPT partitioned boot media.

# |- (bootloader)
# |- user/libc
# |- user/*
# |- kernel
# |- boot media

set( LensorOS_VERSION   0.0.2 )
set( LensorOS_LANGUAGES ASM ASM_NASM C CXX )


# Use `ccache` if it is installed in system's PATH.
find_program( CCACHE_PROGRAM ccache )
if ( CCACHE_PROGRAM )
  set_property( GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}" )
endif ()

# Use LensorOS GNU-based toolchain by default.
set( CMAKE_TOOLCHAIN_FILE toolchain/lensor_gnu_toolchain.cmake)
include( toolchain/config.cmake )
if ( ARCH STREQUAL "" )
  message(FATAL_ERROR "[31;5mERROR: `toolchain/config.cmake` MUST SET `ARCH` variable to non-empty string![m")
endif()

cmake_minimum_required( VERSION 3.20 )
project( LensorOS VERSION ${LensorOS_VERSION} LANGUAGES ${LensorOS_LANGUAGES} )

# If on Unix, add `bootloader`, `toolchain`, and `win_toolchain` targets.
if (CMAKE_HOST_UNIX)
  add_custom_target(
    bootloader
    COMMAND make
    COMMAND make bootloader
    WORKING_DIRECTORY gnu-efi
    USES_TERMINAL
    VERBATIM
  )
  add_custom_target(
    toolchain
    COMMAND bash toolchain/toolchain.sh
    USES_TERMINAL
    VERBATIM
  )
  add_custom_target(
    win_toolchain
    COMMAND bash toolchain/win_toolchain.sh
    USES_TERMINAL
    VERBATIM
  )
endif()

add_subdirectory( user EXCLUDE_FROM_ALL )
add_subdirectory( kernel EXCLUDE_FROM_ALL )

# NOTE: fake dependency just to get stuff to build in the right order.
add_dependencies( Kernel userspace )
add_custom_target(
  kernel-userspace ALL
  DEPENDS
  Kernel
)


set( REPO_DIR ${CMAKE_CURRENT_LIST_DIR} )
set( SCRIPTS_DIR "${REPO_DIR}/scripts" )
cmake_path(
  NATIVE_PATH
  SCRIPTS_DIR
  NORMALIZE
  SCRIPTS_DIR
)
set( IMAGE_DIR "${REPO_DIR}/bin" )
cmake_path(
  NATIVE_PATH
  IMAGE_DIR
  NORMALIZE
  IMAGE_DIR
)

# Boot media generation: raw FAT32 images.
add_custom_target(
  image_raw
  COMMAND ${CMAKE_COMMAND} -P ${SCRIPTS_DIR}/mkfat32images.cmake
  USES_TERMINAL
  VERBATIM
)
# Image generation relies on kernel *and* userspace
add_dependencies( image_raw kernel-userspace )

# Boot media generation: GPT partitioned hard drive.
find_program( CREATEGPT_PROGRAM createGPT )
if( CREATEGPT_PROGRAM )
  message(
    VERBOSE
    "Found createGPT, creating target: image_gpt"
  )
  add_custom_target(
    image_gpt ALL
    COMMAND
    ${CREATEGPT_PROGRAM}
    -o ${IMAGE_DIR}/LensorOS.bin
    -p ${IMAGE_DIR}/LensorOS.img --type system
    -p ${IMAGE_DIR}/LensorOSData.img --type 00000000-0000-0000-0000-000000000069 --name LensorOSData
    DEPENDS
    image_raw
    COMMENT "Generating GUID Partion Table with EFI System partion using `createGPT`"
    USES_TERMINAL
    VERBATIM
  )
else()
  message(NOTICE "[31;5mWARN: MISSING PROGRAM!  Could not find `createGPT`, image_gpt target has not been generated. See dependencies in README.[m")
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
  if( TARGET image_raw )
    add_dependencies( image_iso image_raw )
  endif()
else()
  message(NOTICE "[31;5mWARN: MISSING PROGRAM!  Could not find `xorriso`, image_iso target has not been generated. See dependencies in README.[m")
endif()

# Boot media generation: GPT formatted disk image in VDI format.
find_program( QEMU_IMG_PROGRAM qemu-img )
if( QEMU_IMG_PROGRAM )
  message(
    VERBOSE
    "Found qemu-img, creating target: image_vdi"
  )
  add_custom_target(
    image_vdi
    COMMAND ${QEMU_IMG_PROGRAM} convert -O vdi LensorOS.bin LensorOS.vdi
    COMMENT "Generating VDI format of GUID Partion Table with EFI System partition using `qemu_img`"
    WORKING_DIRECTORY ${IMAGE_DIR}
    USES_TERMINAL
    VERBATIM
  )
  if( TARGET image_gpt )
    add_dependencies( image_vdi image_gpt )
  endif()
else()
  message(NOTICE "[31;5mWARN: MISSING PROGRAM!  Could not find `qemu-img`, image_vdi target has not been generated. See dependencies in README.[m")
endif()

# Add a custom target to run QEMU with the proper flags
# if QEMU for ARCH is found in the PATH environment variable.
find_program( QEMU_PROGRAM qemu-system-${ARCH} )
if ( QEMU_PROGRAM )
  message(
    VERBOSE
    "Found QEMU for ${ARCH}, creating targets:\r\n"
    "|-- run_qemu\r\n"
    "|-- runimg_qemu\r\n"
    "|-- runhda_qemu\r\n"
    "`-- runiso_qemu"
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
    # One hundred and twenty eight megabytes of RAM.
    -m 128M
    # Use local time as real time clock base.
    -rtc base=localtime,clock=host,driftfix=none
    # No networking drivers yet.
    -net none
    # Show extra information regarding triple faults.
    -d cpu_reset
    # Use stdio as serial input and output.
    # This allows debug messages to reach the terminal.
    -serial stdio
  )
  if ( CMAKE_HOST_WIN32 )
    list( APPEND QEMU_FLAGS
      -audiodev dsound,id=audio_device
      -machine pcspk-audiodev=audio_device
    )
  else()
    list( APPEND QEMU_FLAGS
      -audiodev oss,id=audio_device
      -machine pcspk-audiodev=audio_device
    )
  endif()
  if( QEMU_DEBUG )
    list( APPEND QEMU_FLAGS -S -s )
  endif()
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
  file(
    COPY_FILE
    "${REPO_DIR}/OVMFbin/OVMF_VARS-pure-efi.fd"
    "${OVMF_VARS}"
  )
  set( FAT_BOOT_DIR "${IMAGE_DIR}/fat" )
  add_custom_target(
    run_qemu
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FAT_BOOT_DIR}/EFI/BOOT"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FAT_BOOT_DIR}/LensorOS"
    COMMAND ${CMAKE_COMMAND} -E copy
    "${REPO_DIR}/gnu-efi/x86_64/bootloader/main.efi" "${FAT_BOOT_DIR}/EFI/BOOT/"
    COMMAND ${CMAKE_COMMAND} -E copy
    "${SCRIPTS_DIR}/startup.nsh" "${FAT_BOOT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy
    "${IMAGE_DIR}/kernel.elf" "${FAT_BOOT_DIR}/LensorOS/"
    COMMAND ${CMAKE_COMMAND} -E copy
    "${IMAGE_DIR}/dfltfont.psf" "${FAT_BOOT_DIR}/LensorOS/"
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -hdb fat:floppy:rw:${FAT_BOOT_DIR}
    COMMAND ${CMAKE_COMMAND} -E rm -R ${FAT_BOOT_DIR}
    COMMENT "Running QEMU for ${ARCH} from directory treated as FAT32 floppy"
    USES_TERMINAL
    WORKING_DIRECTORY ${REPO_DIR}
    DEPENDS
    ${REPO_DIR}/gnu-efi/x86_64/bootloader/main.efi
    ${SCRIPTS_DIR}/startup.nsh
    VERBATIM
  )
  add_custom_target(
    runimg_qemu
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -drive format=raw,file=${IMAGE_DIR}/LensorOS.img
    COMMENT "Running QEMU for ${ARCH} from FAT32 `LensorOS.img`"
    USES_TERMINAL
    WORKING_DIRECTORY ${REPO_DIR}
    VERBATIM
  )
  add_custom_target(
    runhda_qemu
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -drive format=raw,file=${IMAGE_DIR}/LensorOS.bin
    COMMENT "Running QEMU for ${ARCH} from GPT `LensorOS.bin`"
    USES_TERMINAL
    WORKING_DIRECTORY ${REPO_DIR}
    VERBATIM
  )
  add_custom_target(
    runiso_qemu
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -drive format=raw,file=${IMAGE_DIR}/LensorOS.iso,media=cdrom
    COMMENT "Running QEMU for ${ARCH} from ISO `LensorOS.iso`"
    USES_TERMINAL
    WORKING_DIRECTORY ${REPO_DIR}
    VERBATIM
  )
  # Dependencies
  add_dependencies( run_qemu Kernel )
  if( TARGET image_raw )
    add_dependencies( runimg_qemu image_raw )
  endif()
  if( TARGET image_gpt )
    add_dependencies( runhda_qemu image_gpt )
  endif()
  if( TARGET image_iso )
    add_dependencies( runiso_qemu image_iso )
  endif()
else()
  message(
    "-- QEMU for ${ARCH} not found on your system, skipping run[img|gpt|iso]_qemu target generation."
  )
endif()

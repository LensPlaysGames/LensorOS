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
# along with LensorOS. If not, see <https://www.gnu.org/licenses/>


# HERES THE IDEA
# cmake --build bld will generate GPT partitioned boot media.

# |- (bootloader)
# |- user/libc
# |- user/*
# |- kernel
# |- boot media


set( LensorOS_VERSION   0.0.2 )
set( LensorOS_LANGUAGES ASM ASM_NASM C CXX )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set( CMAKE_C_STANDARD 23 )
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# Use `ccache` if it is installed in system's PATH.
find_program( CCACHE_PROGRAM ccache )
if ( CCACHE_PROGRAM )
  set_property( GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}" )
endif ()

# Use LensorOS GNU-based toolchain by default.
if ( NOT CMAKE_TOOLCHAIN_FILE )
  set( CMAKE_TOOLCHAIN_FILE toolchain/lensor_gnu_toolchain.cmake)
endif()

include( toolchain/config.cmake )
if ( ARCH STREQUAL "" )
  message(FATAL_ERROR "[31;5mERROR: `toolchain/config.cmake` MUST SET `ARCH` variable to non-empty string![m")
endif()
add_compile_definitions(${ARCH})

include_directories("${CMAKE_CURRENT_LIST_DIR}/def/")

cmake_minimum_required( VERSION 3.20 )
project( LensorOS VERSION ${LensorOS_VERSION} LANGUAGES ${LensorOS_LANGUAGES} )

# If on Unix, add `bootloader`, `toolchain`, and `win_toolchain` targets.
if (CMAKE_HOST_UNIX)
  add_custom_target(
    bootloader
    COMMAND make
    COMMAND make bootloader
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/gnu-efi
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
add_subdirectory( ports EXCLUDE_FROM_ALL )
add_subdirectory( kernel EXCLUDE_FROM_ALL )
set_target_properties( Kernel PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/boot/LensorOS )

# NOTE: fake dependency just to get stuff to build in the right order.
add_dependencies( Kernel userspace )
add_custom_target(
  kernel-userspace ALL
  DEPENDS
  Kernel
)
add_custom_target(
  install-bootloader
  COMMAND
  ${CMAKE_COMMAND} -E copy
  "${CMAKE_CURRENT_LIST_DIR}/bin/boot_${ARCH}.efi"
  "${CMAKE_CURRENT_LIST_DIR}/boot/EFI/BOOT/main.efi"
  COMMENT "Installed ${ARCH} UEFI Bootloader"
  USES_TERMINAL
  VERBATIM
)
add_dependencies( kernel-userspace install-bootloader )


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
  BYPRODUCTS
  ${IMAGE_DIR}/LensorOS.img
  ${IMAGE_DIR}/LensorOSData.img
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
      # Show extra information regarding triple faults.
      # NOTE: "int" is extremely noisy, literal gigabytes of text
      # -d int,cpu_reset
      -d cpu_reset,invalid_mem
      # -D "${CMAKE_CURRENT_LIST_DIR}/qemu.log"
      -no-reboot -no-shutdown
      # Use stdio as serial input and output.
      # This allows debug messages to reach the terminal.
      -serial stdio
  )
  if (${ARCH} MATCHES "x86_64")
    list( APPEND QEMU_FLAGS
      -machine q35
      # cpu: Broadwell, Cascadelake-Server, Cooperlake, Conroe, core2duo,
      #      Denverton, Dhyana, EPYC, Haswell, IvyBridge, kvm64, max,
      #      Nehalem, Penryn, qemu64, SandyBridge, Skylake-[Client|Server],
      #      Snowridge, Westmere
      -cpu qemu64
      # Multiple processors (commented until utilized).
      #-smp 8,sockets=2,cores=2,threads=2,maxcpus=8
      -m 256M
      # Use local time as real time clock base.
      -rtc base=localtime,clock=host,driftfix=none
      # e1000 network card emulation
      -nic user,ipv6=off,model=e1000,mac=52:54:98:76:54:32
      # Intel HDA 1.0 Sound
      -device intel-hda,id=sound0,msi=on
      -device hda-duplex,id=codec0,bus=sound0.0,audiodev=audio0
      -machine pcspk-audiodev=audio0
    )

    set( OVMF_CODE "${REPO_DIR}/OVMFbin/OVMF_CODE-pure-efi.fd" )
    set( OVMF_VARS "${REPO_DIR}/OVMFbin/OVMF_VARS-pure-efi.fd" )

  elseif(${ARCH} MATCHES "aarch64")
    list( APPEND QEMU_FLAGS
      -machine virt,acpi=off
      -nodefaults
      -monitor vc
      # CPU:
      # a64fx, arm1026, arm1136, arm1136-r2, arm1176, arm11mpcore, arm926,
      # arm946, cortex-a15, cortex-a35, cortex-a53, cortex-a55, cortex-a57,
      # cortex-a7, cortex-a710, cortex-a72, cortex-a76, cortex-a78ae, cortex-
      # a8, cortex-a9, cortex-m0, cortex-m3, cortex-m33, cortex-m4, cortex-m55,
      # cortex-m7, cortex-r5, cortex-r52, cortex-r5f, max, neoverse-n1,
      # neoverse-n2, neoverse-v1, sa1100, sa1110, ti925t
      -cpu cortex-a53
      # Multiple processors (commented until utilized).
      #-smp 4
      -m 512M
      -device ramfb
    )

    if (NOT EXISTS "${REPO_DIR}/OVMFbin/OVMF_CODE-aarch64.fd")
      # Fedora (presumably others, too) provides DNF system packages to provide
      # UEFI firmware for QEMU testing for various archictectures.
      #
      # Red Hat:
      #     sudo dnf install edk2-aarch64
      #
      # Debian:
      #     sudo apt install qemu-efi-aarch64
      #
      # NOTE (Windows): "C:\Program Files\qemu\share\" or something
      #
      # You can also follow instructions inside OVMFbin/README.md, modifying
      # x64 specific stuff to aarch64 as needed, to produce these files yourself.
      if (EXISTS "/usr/share/edk2/aarch64/QEMU_EFI-pflash.raw")
        file(
          COPY_FILE
          "/usr/share/edk2/aarch64/QEMU_EFI-pflash.raw"
          "${REPO_DIR}/OVMFbin/OVMF_CODE-aarch64.fd"
        )
        file(
          COPY_FILE
          "/usr/share/edk2/aarch64/QEMU_EFI-qemuvars-pflash.raw"
          "${REPO_DIR}/OVMFbin/OVMF_VARS-aarch64.fd"
        )
      elseif (EXISTS "/usr/share/AAVMF/AAVMF_CODE.fd")
        file(
          COPY_FILE
          "/usr/share/AAVMF/AAVMF_CODE.fd"
          "${REPO_DIR}/OVMFbin/OVMF_CODE-aarch64.fd"
        )
        file(
          COPY_FILE
          "/usr/share/AAVMF/AAVMF_VARS.fd"
          "${REPO_DIR}/OVMFbin/OVMF_VARS-aarch64.fd"
        )
      else()
        message(
          FATAL_ERROR
          "Could not find QEMU UEFI EDK2 Firmware for ${ARCH}"
        )
      endif()
    endif()

    set( OVMF_CODE "${REPO_DIR}/OVMFbin/OVMF_CODE-aarch64.fd" )
    set( OVMF_VARS "${REPO_DIR}/OVMFbin/OVMF_VARS-aarch64.fd" )
  else()
    message(
      FATAL_ERROR
      "unrecognized architecture (QEMU flags): QEMU:'${QEMU_PROGRAM}' ARCH:${ARCH}"
    )
  endif()

  # Hardware Acceleration
  # Only possible if guest architecture is same as host (i.e. x86_64
  # LensorOS running on QEMU running on an x86_64 machine).
  if ( CMAKE_HOST_WIN32 )
    list( APPEND QEMU_FLAGS
      -accel whpx
    )
  elseif( CMAKE_HOST_APPLE )
    list( APPEND QEMU_FLAGS
      -accel hvf
    )
  elseif( CMAKE_HOST_UNIX )
    list( APPEND QEMU_FLAGS
      -enable-kvm
    )
  endif()
  # list( APPEND QEMU_FLAGS
  #   -accel tcg,thread=multi
  # )

  # Audio/Speaker Setup
  if ( CMAKE_HOST_WIN32 )
    list( APPEND QEMU_FLAGS
      -audiodev dsound,id=audio0
    )
  else()
    list( APPEND QEMU_FLAGS
      -audiodev pa,id=audio0
    )
  endif()

  if( QEMU_DEBUG )
    list( APPEND QEMU_FLAGS -S -s )
  endif()

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
    dbghda_qemu
    COMMAND # TODO: open new shell window and run gdb in it
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -drive format=raw,file=${IMAGE_DIR}/LensorOS.bin
    -S -s
    COMMENT "Running QEMU(Debug) for ${ARCH} from GPT `LensorOS.bin`"
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
  add_custom_target(
    runusb_qemu
    COMMAND ${QEMU_PROGRAM} ${QEMU_FLAGS}
    -machine usb=on
    -device qemu-xhci,id=xhci
    -device usb-storage,bus=xhci.0,drive=usbdisk,removable=on
    -drive if=none,id=usbdisk,format=raw,file=${IMAGE_DIR}/LensorOS.bin
  )
  # Dependencies
  add_dependencies( run_qemu Kernel )
  if( TARGET image_raw )
    add_dependencies( runimg_qemu image_raw )
  endif()
  if( TARGET image_gpt )
    add_dependencies( runhda_qemu image_gpt )
    add_dependencies( dbghda_qemu image_gpt )
    add_dependencies( runusb_qemu image_gpt )
  endif()
  if( TARGET image_iso )
    add_dependencies( runiso_qemu image_iso )
  endif()
else()
  message(
    "-- QEMU for ${ARCH} not found on your system, skipping run[img|gpt|iso]_qemu target generation."
  )
endif()

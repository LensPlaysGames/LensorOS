# Run like `cmake -P mkfat32efibootimage.cmake`

set(REPO_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
set(IMAGE_DIR ${REPO_DIR}/bin)
set(SCRIPTS_DIR ${REPO_DIR}/scripts)

# Boot media generation: raw FAT32 images.
# TODO: Can also use truncate in place of dd
find_program( DD_PROGRAM dd )
find_program( MTOOLS_PROGRAM mtools )
if(NOT DD_PROGRAM OR NOT MTOOLS_PROGRAM)
  message(NOTICE "[31;5mWARN: MISSING PROGRAM!  Could not find `dd` and `mtools`, cannot generate FAT32 UEFI boot media. See dependencies in README.[m")
endif()

file(
  COPY_FILE
  ${REPO_DIR}/kernel/res/dfltfont.psf
  ${IMAGE_DIR}/dfltfont.psf
)

function(mcopy_file filename_to_copy directory)
  if (NOT EXISTS ${filename_to_copy})
    message(FATAL_ERROR "File at " ${filename_to_copy} " MUST exist to generate UEFI boot media")
  endif()
  execute_process(COMMAND mcopy -i ${IMAGE_DIR}/LensorOS.img ${filename_to_copy} ::${directory})
endfunction()

message(STATUS "Generating FAT32 UEFI boot media...")
file(MAKE_DIRECTORY ${IMAGE_DIR})
execute_process(COMMAND ${DD_PROGRAM} if=/dev/zero of=${IMAGE_DIR}/LensorOS.img count=93750)
execute_process(COMMAND mformat -i ${IMAGE_DIR}/LensorOS.img -F -v "EFI System" ::)
execute_process(COMMAND mmd -i ${IMAGE_DIR}/LensorOS.img ::/EFI)
execute_process(COMMAND mmd -i ${IMAGE_DIR}/LensorOS.img ::/EFI/BOOT)
execute_process(COMMAND mmd -i ${IMAGE_DIR}/LensorOS.img ::/LensorOS)
mcopy_file(${REPO_DIR}/gnu-efi/x86_64/bootloader/main.efi /EFI/BOOT)
mcopy_file(${SCRIPTS_DIR}/startup.nsh "")
mcopy_file(${IMAGE_DIR}/kernel.elf /LensorOS)
mcopy_file(${IMAGE_DIR}/dfltfont.psf /LensorOS)

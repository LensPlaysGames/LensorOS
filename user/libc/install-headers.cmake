function(install_headers)
    set(dest_dir $ENV{SOURCE_DIR}/../../root/inc)

    file(GLOB libc_headers "$ENV{SOURCE_DIR}/*.h")
    file(GLOB libc_bits_headers "$ENV{SOURCE_DIR}/bits/*.h")
    file(GLOB libc_sys_headers "$ENV{SOURCE_DIR}/sys/*.h")
    file(GLOB libcxx_headers "$ENV{SOURCE_DIR}/../../std/*")
    file(GLOB libcxx_bits_headers "$ENV{SOURCE_DIR}/../../std/bits/*.h")

    file(COPY ${libc_headers} DESTINATION "${dest_dir}")
    file(COPY ${libc_bits_headers} DESTINATION "${dest_dir}/bits")
    file(COPY ${libc_sys_headers} DESTINATION "${dest_dir}/sys")
    file(COPY ${libcxx_headers} DESTINATION "${dest_dir}")
    file(COPY ${libcxx_bits_headers} DESTINATION "${dest_dir}/bits")
endfunction()

install_headers()
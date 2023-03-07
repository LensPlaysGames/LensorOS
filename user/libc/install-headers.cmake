function(install_headers)
    set(dest_dir ${SOURCE_DIR}/../../root/inc)

    file(GLOB libc_headers        "${SOURCE_DIR}/*.h")
    file(GLOB libc_bits_headers   "${SOURCE_DIR}/bits/*.h")
    file(GLOB libc_sys_headers    "${SOURCE_DIR}/sys/*.h")
    file(GLOB libcxx_headers      "${SOURCE_DIR}/../../std/*")
    file(GLOB libcxx_bits_headers "${SOURCE_DIR}/../../std/bits/*.h")

    file(COPY ${libc_headers}        DESTINATION "${dest_dir}")
    file(COPY ${libc_bits_headers}   DESTINATION "${dest_dir}/bits")
    file(COPY ${libc_sys_headers}    DESTINATION "${dest_dir}/sys")
    file(COPY ${libcxx_headers}      DESTINATION "${dest_dir}")
    file(COPY ${libcxx_bits_headers} DESTINATION "${dest_dir}/bits")
endfunction()

install_headers()


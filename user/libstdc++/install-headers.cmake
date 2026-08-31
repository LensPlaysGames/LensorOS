function(install_headers)
    set(dest_dir ${SOURCE_DIR}/../../root/inc)

    file(GLOB libcxx_headers      "${SOURCE_DIR}/../../std/include/*")
    file(GLOB libcxx_bits_headers "${SOURCE_DIR}/../../std/include/bits/*.h")

    file(COPY ${libcxx_headers}      DESTINATION "${dest_dir}")
    file(COPY ${libcxx_bits_headers} DESTINATION "${dest_dir}/bits")
endfunction()

install_headers()

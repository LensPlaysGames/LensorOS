# ASSUMES NINJA BUILD SYSTEM AND WORKING DIRECTORY OF ROOT OF REPOSITORY

PROC=$(nproc)

# SYSTEM LIBRARIES
cmake -G Ninja -S user/libc -B user/libc/winja && cmake --build user/libc/winja --clean-first -- -j $PROC
# USERSPACE PROGRAMS
cmake -G Ninja -S user/blazeit -B user/blazeit/winja \
    && cmake --build user/blazeit/winja --clean-first -- -j $PROC \
    && xcopy /y user\blazeit\winja\blazeit user\blazeit\blazeit

cmake -G Ninja -S user/stdout -B user/stdout/winja \
    && cmake --build user/stdout/winja --clean-first -- -j $PROC \
    && xcopy /y user\stdout\winja\stdout user\stdout\stdout

cmake -G Ninja -S user/pwd -B user/pwd/winja \
    && cmake --build user/pwd/winja --clean-first -- -j $PROC \
    && xcopy /y user\pwd\winja\pwd user\pwd\pwd

# KERNEL
cmake -G Ninja -S kernel -B kernel/winja -DHIDE_UART_COLOR_CODES=OFF \
    && cmake --build kernel/winja --target runhda_qemu --clean-first -- -j $PROC

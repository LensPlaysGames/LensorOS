# Fedora Build Process

## Dependencies

``` shell
sudo dnf install binutils clang cmake g++ gcc git lld make nasm qemu qemu-system-aarch64 qemu-system-x86_64
```

NOTE: `clang`/`lld` required either for `clang` toolchain (only required by certain **optional** third-party ports).

You'll need `createGPT` as well; find it [here](https://github.com/LensPlaysGames/createGPT).

## Bootloader
(gnu-efi)
``` shell
cd gnu-efi
make
make bootloader
cd ..
```

(TODO: RADII)

## Toolchain

[Build a custom GCC](./toolchain/README.md#building-the-toolchain)

Alternatively, use LLVM's `clang`.

## The Rest of the Circle

``` shell
cmake -B bld --toolchain=./toolchain/lensor_gnu_toolchain.cmake
```

**If using `clang` toolchain**, be sure to swap `gnu` for `clang`, i.e.
``` shell
cmake -B bld --toolchain=./toolchain/lensor_clang_toolchain.cmake
```

Then, launch QEMU into LensorOS (after building *all the things*).
Optionally pass `--parallel` for parallelized build (it's faster).
``` shell
cmake --build bld --target runhda_qemu
```

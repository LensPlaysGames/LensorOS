# LensorOS Standard `C` Library
A homebrew standard `C` library for the userland of LensorOS.

### Building
To build the LensorOS Standard `C` Library (really needs a
  name), the LensorOS toolchain needs to be built already.

For further instructions, [see the toolchain README](/toolchain/README.md).

#### Building the Bootstrap Libraries
NOTE: This step does **not** need to be completed, as **these binaries are provided *pre-built* with the repository**.

For startup and destruction of programs in the standard `C` library, a few different libraries are utilized:
- crt0.o
- crti.o
- crtn.o

Use the freestanding LensorOS toolchain compiler to generate object files from the corresponding assembly files:
```bash
x86_64-lensor-gcc -c /Path/to/LensorOS/user/libc/crt0.s
x86_64-lensor-gcc -c /Path/to/LensorOS/user/libc/crti.s
x86_64-lensor-gcc -c /Path/to/LensorOS/user/libc/crtn.s
```

Once the object files have been generated, copy them into the
  system libraries directory within the sysroot (`/lib`).
  Alternatively, one could use the `-o` option to specify the correct output path.

### Build the `C` Library for LensorOS
Generate a build system using [CMake](https://www.cmake.org), then invoke it:
```bash
cd /Path/to/LensorOS/user/libc/
cmake -S . -B bld
cmake --build bld
```

This will generate a `libc.a` library file that may be stored in the
  system library directory within the sysroot (`/lib`).
From here, if a program is built that links to the standard `C` library,
  it will link to the LensorOS implementation, allowing the
  generated executable to be run from LensorOS in userland.

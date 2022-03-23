# LensorOS Standard C Library
A homebrew standard `C` library for the userland of LensorOS.

### Building the LensorOS Toolchain for Userland
By building the toolchain for userland, it will be able to compile and link `C` programs that can run within LensorOS.

The LensorOS toolchain can be built without any standard `C` library, however this prevents the building of userland programs, a.k.a. programs that can run within LensorOS (the kernel may still be built without any `C` library). 

Building the toolchain that supports a freestanding environment (with no `C` library) is necessary before beginning the toolchain build for userland (see [README.toolchain.md](/toolchain/README.toolchain.md)).

#### Ensure a Sysroot Directory Exists
For this tutorial, the sysroot directory is located within the LensorOS repository, and named `root`. Technically this directory could be anywhere permissions allow, but keeping it in the LensorOS directory helps stay organized.

If you are unsure of what exactly a "sysroot" is, I'll do my best to explain, and also link resources that I found helpful.

Resources:
- [StackOverflow Thread](https://stackoverflow.com/questions/39920712/what-is-a-sysroot-exactly-and-how-do-i-create-one)
- [GCC Documentation](https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html) (search for `--sysroot`)

When compiling a program, the generated executable needs to use the c library of the target platform. In the case of developing for LensorOS, most of the time the host is different than the target (or, in other words, the OS of the machine running the compiler is different than the OS running the compiled program). This means if we use a compiler not built specifically for the proper OS, the links to the standard library will all be incorrect.

To remedy this issue, the compiler allows a sysroot to be specified. Sysroot is short for system root. It acts as a representation of the root filesystem of LensorOS. This allows the compiler to find the proper OS-specific libraries and headers, even when building from an OS that isn't LensorOS.

Now that the sysroot exists, let's begin populating it with LensorOS headers and libraries.

Create a directory named `usr`.

Create two directories within the `usr` directory:
- `include`
- `lib`

The final sysroot layout should look something like this:
```
root
`-- usr
    |-- include
    `-- lib
```

#### Copy header files
Copy all of the `.h` header files from this `libc` directory and subdirectory `sys` into the system header directory in the sysroot (`/usr/include`). For a list of all headers that need copied, see the `HEADERS` variable in [CMakeLists.txt](CMakeLists.txt).

#### Bootstrap the Basic Libraries
For startup and destruction of programs in the standard `C` library, a few different libraries are utilized:
- crt0.o
- crti.o
- crtn.o

Use the LensorOS toolchain compiler to generate object files from the corresponding assembly files:
```bash
x86_64-lensoros-elf-gcc -c /Path/to/LensorOS/user/libc/crt0.s
x86_64-lensoros-elf-gcc -c /Path/to/LensorOS/user/libc/crti.s
x86_64-lensoros-elf-gcc -c /Path/to/LensorOS/user/libc/crtn.s
```

Once the object files have been generated, copy them into the system libraries directory within the sysroot (`/usr/lib`). Alternatively, one could use the `-o` option to specify the correct output path.

#### Build the LensorOS Toolchain with Sysroot Specified
With the headers and bootstrap objects in place, it's time to actually compile the userland toolchain.

Follow all the instructions found in the [toolchain README](/toolchain/README.toolchain.md), with a (few) minor change(s).

When configuring (ie. running the configure script for binutils, GCC), use the following flag(s):
```bash
--with-sysroot="/Absolute/Path/to/LensorOS/root"
```

If the `make all-target-libgcc` step gives errors, it's likely an error in the generation and arrangement of the sysroot.

### Build the `C` Library for LensorOS
Once the toolchain that supports a sysroot is built (libgcc has been built successfully), it's time to build this `C` library itself, and put that in the sysroot.

Generate a build system using [CMake](https://www.cmake.org), then invoke it:
```bash
cd /Path/to/LensorOS/user/libc/
cmake -S . -B bld
cd bld
make
```

This will generate a `libc.a` library file that may be stored in the system library directory within the sysroot (`/usr/lib`). From here, if a program is built that links to the standard `C` library, it will link to the LensorOS implementation, allowing the generated executable to be run from LensorOS in userland.

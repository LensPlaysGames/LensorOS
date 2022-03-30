# The LensorOS toolchain
LensorOS is compiled with a special version of the GNU Compiler Collection (GCC).

This special version allows for a host machine that is not running LensorOS to build programs that may run on LensorOS. This means a different OS (environment) can be used to to develop in, rather than having to develop LensorOS from within LensorOS (which complicates things).

## Building the LensorOS toolchain
If you are on Windows, you are really going to have a hard time (not to say it isn't possible). However, everything can 100% be done through the Windows Subsystem for Linux, and that's what I recommend.

Building the compiler does take quite some time (15-90min, or more), as well as ~5GB of hard drive space. You have been warned. \
The good part is this only needs to be done once (in a blue moon).

In a linux terminal, first install the dependencies:
```bash
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
```

A script is included that will do all of the following download, configure, and build steps automatically; run it with bash:
```bash
bash toolchain.sh
```
Once complete, you'll find the `toolchain/cross/` directory of the repo has been filled with binaries, libraries, and documentation on a LensorOS cross compiler. This compiler runs on an `x86_64-linux-gnu` machine, and generates freestanding executables for the `x86_64-lensoros-elf` target.

To generate hosted programs for LensorOS (userland/user-space), the LensorOS `C` library must be built and integrated into the toolchain. \
[For further instructions on compiling userland executables for LensorOS, see the libc README](/user/libc/README.md).

#### 1.) Obtain the source code of the following GNU packages
[GNU Compiler Collection](https://www.gnu.org/software/gcc/) **Version 11.2.0** \
[Binutils](https://www.gnu.org/software/binutils/) **Version 2.38**

I recommend downloading the source code archives into this toolchain directory. \
Choose a mirror that is closest to you and gives reasonable download speeds.
```bash
curl https://mirrors.kernel.org/gnu/binutils/binutils-2.38.tar.xz --output binutils-2.38.tar.xz
curl https://mirrors.kernel.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz --output gcc-11.2.0.tar.xz
```

The download will be compressed; extract the archives to get the source code:
```bash
tar -xf binutils-2.38.tar.xz -C .
tar -xf gcc-11.2.0.tar.xz -C .
```

NOTE: It is possible to over-ride your system's compiler collection and/or binutils with the cross-compiler, breaking your system's default host compiler. Stay away from any directories that do not derive from `$HOME`!

#### 2.) Configuring GCC to build `libgcc` with no red zone
A patch file for GCC is included in the unified diff format. Simply invoke `patch` like so from the toolchain directory (assuming `gcc-11.2.0` is a subdirectory with GCC source code extracted inside of it, as shown above).
```bash
patch -s -u -p0 < gcc-11.2.0.patch
```

Alternatively, here's how to create the files manually.

Create an extensionless file named `t-x86_64-elf` within the GCC source code at `gcc-11.2.0/gcc/config/i386/`, and save the following within that file:
```
MULTILIB_OPTIONS += mno-red-zone
MULTILIB_DIRNAMES += no-red-zone
```

This adds a new multilib configuration to `libgcc` that doesn't use a red zone.

By default this newly created configuration will not be used, so we must also edit `gcc/config.gcc`.

Find the following lines:
```
x86_64-*-elf*)
    tm_file="${tm_file} i386/unix.h i386/att.h dbxelf.h elfos.h newlib-stdint.h i386/i386elf.h i386/x86-64.h"
    ;;
```
And replace them with:
```
x86_64-*-elf*)
    tmake_file="${tmake_file} i386/t-x86_64-elf" # Add multilib configuration with no red zone
    tm_file="${tm_file} i386/unix.h i386/att.h dbxelf.h elfos.h newlib-stdint.h i386/i386elf.h i386/x86-64.h"
    ;;
```

With that, in the coming steps, GCC will build `libgcc` without a red zone. This allows the `-mno-red-zone` flag to work correctly.

#### 3.) Setup environment variables
These variables are used a few times within the next steps, so saving them here prevents simple typos from getting in the way.

`PREFIX` is set to the absolute path where the final toolchain build will reside.
`TARGET` is set to the target triplet of the generated compiler.

First, create a directory where the final output executables and libraries will be installed. It is recommended to put this in the current toolchain directory.
```bash
mkdir -p cross
```

Now, define the variables:
```bash
export PREFIX="Path/to/LensorOS/toolchain/cross/"
export TARGET=x86_64-lensoros-elf
```

#### 4.) Configure Binutils
Create a new subdirectory within `toolchain` named `build-binutils`, or similar.
```bash
mkdir build-binutils
cd build-binutils
```

Next, from within that newly created directory, run the configure script supplied by the Binutils source code.
```bash
../binutils-2.38/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
```

Flags:
- `--with-sysroot` tells binutils to enable sysroot support (eventually we will add `=Absolute/Path/to/LensorOS/sysroot` or some other path that points to the OS-supplied libraries).
- `--disable-nls` disabled binutils' native language support. This cuts down on build size and time.
- `--disable-werror` allows compilation to continue in the event of a warning (I usually don't get any, but a warning is no reason to stop a 5+ minute compilation).

#### 5.) Build Binutils
NOTE: Anytime you see a `make` command being issued, you can speed it up if you have multiple cores on your CPU using the `-j` option. For example, on a 4-core CPU, running `make target -j4` would run recipes in parallel on all cores of the CPU at the same time, significantly decreasing build times.

Within the `toolchain/build-binutils/` directory, and after configuration, run the following:
```bash
make
make install
```

You should now have a working version of GNU's Binutils installed at `$PREFIX`.

#### 6.) Prepare the GNU Compiler Collection
First, GCC has some pre-requisites that must be downloaded. \
Luckily, the source code comes with an easy-to-use binary that will download them for us.
```bash
cd gcc-11.2.0
./contrib/download_prerequisites
```

Next, GCC must be configured, much like Binutils.
```bash
cd Path/to/LensorOS/toolchain
mkdir build-gcc
cd build-gcc
../gcc-11.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
```

This should generate a Makefile, among other things, that will be used to build GCC in the next step.

Flags:
- `--disable-nls` disables native language support (English-only reduces build size and times).
- `--enable-languages` disables all other languages except for what is stated here (reduces size and build times).
- `--without-headers` specifies that GCC shouldn't rely on any external C library (standard or runtime) being present on the target.

#### 7.) Build the GNU Compiler Collection
Warning: This step takes a long time. Utilize the `-j` option if you have more than a single core CPU.

Within the `toolchain/build-gcc/` directory, following the previous preparation step, run the following:
```bash
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
```

First, we build the new version of GCC (`all-gcc` target). \
Next, we build `libgcc` for our target. `libgcc` is a very stripped standard C library that the GCC compiler itself uses. We supply this on our target so that things like fixed width integers, booleans, etc can be used within LensorOS source code.

Finally, we install both the new GCC for our host and `libgcc` for our target. With this complete, you should have a working cross compiler that will generate ELF executables for LensorOS.

If you run into any issues, please let me know/make an issue on GitHub. I'll do my best to help you out.

## Using the LensorOS toolchain
To build the kernel, it is required that the cross-compiler executable directory be added to your build system's `PATH` variable.

NOTE: Your build system refers to the system that the build tools reside on. \
If using WSL, ensure you are adding to it's `$PATH`, **not** your host machine. 

#### Temporary
```bash
export PATH="Path/to/LensorOS/toolchain/cross/bin:$PATH"
```

#### Permanent
For exact instructions, see your shell's configuration options.

On most Unix-like systems that are using `BASH`, it's as simple as adding the line in the `temporary` section above to the bottom of your `$HOME/.bashrc` configuration file. \
To use the newly made config file, read it with the `.` command like so: \
```bash
. $HOME/.bashrc
```

Once the `$PATH` variable is successfully configured, running `x86_64-lensoros-elf-gcc` from a shell will use the LensorOS `C` cross compiler.
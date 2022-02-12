# The LensorOS toolchain
LensorOS is compiled with a special version of the GNU Compiler Collection (GCC).

This special version allows for a host machine that is not running LensorOS to build programs for LensorOS. This means you can use a different OS (environment) to develop in, rather than having to develop LensorOS from within LensorOS (which complicates things).

## Building the LensorOS toolchain
GCC relies on GNU's Binutils, so we must build that as well.

Building the compiler does take quite some time, as well as a few GB of hard drive space. \
The good part is this only needs to be done once.

NOTE: Anytime you see a `make` command being issued, you can speed it up if you have multiple cores on your CPU using the `-j` option. For example, on a 4-core CPU, running `make target -j4` would run in parallel on all cores of the CPU at the same time, significantly decreasing build times.

#### 1.) Obtain the source code of the following GNU packages
[GNU Compiler Collection](https://www.gnu.org/software/gcc/) **Version 11.2.0** \
[Binutils](https://www.gnu.org/software/binutils/) **Version 2.38**

I recommend downloading the source code to a directory at `$HOME/cross/src/`, or something similar.

NOTE: It is **not** recommended to over-ride your system's compiler collection and/or binutils with the cross-compiler, otherwise your host compiler may break. Stay away from system directories that do not derive from `$HOME`.

#### 2.) Manually configuring GCC source code to build `libgcc` with no red zone
Ideally, this whole process will be automated. \
For now, this step is very much required.

Create an extensionless file named `t-x86_64-elf` within the GCC source code at `/gcc/config/i386/`, and save the following within that file:
```
# Add libgcc multilib variant without red-zone requirement
 
MULTILIB_OPTIONS += mno-red-zone
MULTILIB_DIRNAMES += no-red-zone
```

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

With that, in the coming steps, GCC will build `libgcc` without a red zone (which is required for x86_64).

#### 3.) Setup environment variables
These variables are used a few times within the next steps, so saving them here prevents simple typos from getting in the way.

Define the following variables:
```bash
export PREFIX="$HOME/cross/"
export TARGET=x86_64-lensoros-elf
```

#### 4.) Configure Binutils
Within the `$HOME/cross/src/` directory, create a new directory named `build-binutils`, or similar.
```
cd $HOME/cross/src/
mkdir build-binutils
cd build-binutils
```

Next, from within that newly created directory, run the configure script supplied with the Binutils source code.
```
../binutils-2.38/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
```

Flags:
- `--with-sysroot` tells binutils to enable sysroot support.
- `--disable-nls` disabled binutils' native language support. This cuts down on build size and time.
- `--disable-werror` allows compilation to continue in the event of a warning.

#### 5.) Build Binutils
Within the `$HOME/cross/src/build-binutils/` directory, after configuration, run the following:
```bash
make
make install
```

You should now have a working version of GNU's Binutils installed at `$PREFIX`.

#### 6.) Prepare the GNU Compiler Collection
First, GCC has some pre-requisites that must be downloaded. \
Luckily, the source code comes with an easy-to-use binary that will download them for us.
```bash
cd $HOME/cross/src/gcc-11.2.0
./contrib/download_prerequisites
```

Next, GCC must be configured, much like Binutils.
```
cd $HOME/cross/src/
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
###### Warning: This step takes a long time.
Within the `$HOME/cross/src/build-gcc/` directory, following the previous preparation step, run the following:
```bash
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
```

First, we build the new version of GCC (`all-gcc` target). \
Next, we build `libgcc` for our target. `libgcc` is a very stripped standard C library that the GCC compiler itself uses. We supply this on our target so that things like fixed width integers, booleans, etc can be used within LensorOS source code.

Finally, we install both the new GCC for our host and `libgcc` for our target. With this complete, you should have a working cross compiler that will generate ELF executables for LensorOS (ie. the kernel :^)

If you run into any issues, please let me know/make an issue on GitHub. I'll do my best to help you out.

## Using the LensorOS toolchain
To build the kernel, it is required that the cross-compiler executable directory be added to your build system's `PATH` variable.

NOTE: Your build system refers to the system that the build tools reside on. \
If using WSL, ensure you are adding to it's `$PATH`, **not** your host machine. 

#### Temporary
```bash
export PATH="$HOME/cross/gcc-11.2.0/bin:$PATH"
```
#### Permanent
For exact instructions, see your shell's configuration options.

On most Unix-like systems that are using `BASH`, it's as simple as adding the line in the `temporary` section above to the bottom of your `$HOME/.bashrc` configuration file. \
To use the newly made config file, read it with the `.` command like so: \
`. $HOME/.bashrc`

Once the `$PATH` variable is successfully configured, simply run `x86_64-lensoros-elf-gcc` from a shell to use the cross compiler.

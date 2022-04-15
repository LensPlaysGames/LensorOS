# The LensorOS Toolchain
LensorOS is compiled with a special version of the GNU Compiler Collection (GCC).

This special version allows for a host machine that is not running
  LensorOS to build programs that may run on LensorOS.
This means a different OS (environment) can be used to to develop in,
  rather than having to develop LensorOS from within LensorOS (which complicates things).
  
---

## Table of Contents
- [Using the Toolchain](#using-the-toolchain)
- [Using a Pre-built Toolchain](#prebuilt-toolchain)
- [Building the Toolchain](#building-the-toolchain)

---

## Using the LensorOS toolchain <a name="using-the-toolchain"></a>
There are two ways for CMake to know how to invoke the cross compiler; only one is required:
1. Ensure the `cross` or `wincross` directory is located in this directory, `/toolchain/`.
2. Add the `cross/bin` or `wincross/bin` directory to the `PATH` environment variable.

---

## Using a Pre-built LensorOS Toolchain <a name="prebuilt-toolchain"></a>
If you happen to be on an `x86_64` machine and also on either Linux or Windows, 
  it's likely there are pre-built binaries for your system available for download. 
  Take a look at the
  [releases page](https://github.com/LensPlaysGames/LensorOS/releases)
  for the latest (pre-)release, which that contains the toolchain as per the license.
  Simply download a toolchain archive corresponding to your OS, then
  extract it and add the `bin` directory to the `PATH` environment variable.
  There is no guarantee that these are up to date with the main branch;
  you have been warned.
  
Do note that using a pre-built toolchain means that the sysroot is not
  made by the toolchain script like normal, and will likely cause errors
  like `Could not find file <stdint.h>` or similar. The `--sysroot` flag
  can be used to point to the proper sysroot, but a proper sysroot must
  be created still. For now, see the [Create a Sysroot](#create-sysroot) step.
  Soon, it will be as simple as running a script to generate the initial sysroot,
  but I haven't gotten around to it yet.

---

## Building the LensorOS Toolchain <a name="building-the-toolchain"></a>
If you are on Windows, you are really going to have a hard
  time if you do not use WSL (not to say it isn't possible).
  Everything can 100% be done through the Windows Subsystem
  for Linux; that's what I recommend.

Building the compiler does take quite some time (15-90min, or more),
  depending on both your internet speed and how powerful your computer is.

The toolchain takes up ~5GB of hard drive space. You have been warned.

In a linux terminal, first install the necessary dependencies:
```bash
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
```

Next, simply run the included `toolchain.sh` script with bash to 
  download, patch, configure, build, and install the toolchain.
```bash
bash toolchain.sh
```
Once complete, you'll find the `/toolchain/cross/` directory of the repo has
  been filled with binaries, libraries, and documentation on a LensorOS cross compiler.
  With the toolchain built, everything within the `/toolchain/` directory,
  except the `/cross/` directory and it's contents, may be deleted.

The `/root/` directory will have also been created, and populated 
  with the bare essentials of the LensorOS root filesystem 
  (namely pre-built `libc` binaries, and some system headers).
  
In order for CMake to use the new toolchain, the `/toolchain/cross/bin/` directory
  of the repo must be added to your system's `PATH` environment variable; 
  [see the Using section](#using-the-toolchain).
  
The generated binaries run on any `x86_64-linux-gnu` machine;
  the cross compiler(s) generate ELF64 executables for `x86_64-lensor` machines.

By default, pre-built `libc` binaries are provided to 
  bootstrap the compiler, allowing us to skip a re-build.
  To *optionally* generate these binaries yourself, 
  [see the libc README](/user/libc/README.md).

#### 1.) Obtain the source code of the following GNU packages
NOTE: The following steps are a manual version of what is accomplished automatically using the `toolchain.sh` script.

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

NOTE: It is possible to over-ride your system's compiler collection 
  and/or binutils with the cross-compiler, breaking your system's default 
  host compiler. Stay away from any directories that do not derive from `$HOME`!

#### 2.) Patch Binutils and GCC
A patch file for Binutils and another for GCC 
  is included in the unified `diff` format. 
Simply invoke `patch` like so from the toolchain directory 
  (assuming `gcc-11.2.0` is a subdirectory with GCC source 
  code extracted inside of it, as shown above).
```bash
patch -s -u -p0 < binutils-2.38-lensor.patch
patch -s -u -p0 < gcc-11.2.0-lensor.patch
```

#### 3.) Create a Sysroot <a name="create-sysroot"></a>
A sysroot, or system root, is a folder that the cross compiler 
  uses as the root filesystem of the target machine. 
Basically, it's an exact copy of the filesystem 
  that is expected to be found on the target.

Create the `root` directory, then copy the `base` directory into it:
```bash
cd /Path/to/LensorOS/
mkdir -p root
cp -r base/* root/
```

The base directory contains pre-built binaries that allow us to skip an initial build of the compiler and jump right to the final build, saving a *lot* of time.

Next, copy all of the `libc` system headers into the sysroot include directory:
```bash
cd /Path/to/LensorOS/user/libc/
find ./ -name '*.h' -exec cp --parents '{}' -t /Path/to/LensorOS/root/inc ';'
```

#### 4.) Setup environment variables
These variables are used a few times within the next steps, so saving them here prevents simple typos from getting in the way.

`SYSROOT` is set to the absolute path of the `root` directory created in the previous step.
`PREFIX` is set to the absolute path where the final toolchain build will reside.
`TARGET` is set to the target triplet of the generated compiler.

Define the variables:
```bash
export SYSROOT="/Path/to/LensorOS/root"
export PREFIX="/Path/to/LensorOS/toolchain/cross"
export TARGET=x86_64-lensor
```

#### 5.) Configure Binutils
Create a new subdirectory within `toolchain` named `build-binutils`, or similar.
```bash
cd /Path/to/LensorOS/toolchain/
mkdir build-binutils
```

At the same time, create a subdirectory for the final install of both Binutils and GCC to be located:
```bash
mkdir -p cross
```

Next, from within the Binutils build directory, run the configure script supplied by the Binutils source code with the following command line flags and options:
```bash
cd build-binutils
../binutils-2.38/configure \
    --target=$TARGET \
	--prefix="$PREFIX" \
	--with-sysroot="$SYSROOT" \
	--disable-nls \
	--disable-werror
```

Flags:
- `--with-sysroot="$SYSROOT"` tells Binutils where to find system headers and libraries.
- `--disable-nls` disables Binutils' native language support. This cuts down on build size and time.
- `--disable-werror` allows compilation to continue in the event of a warning (I usually don't get any, but a warning is no reason to stop a 5-30+ minute compilation).

#### 6.) Build Binutils
NOTE: Anytime you see a `make` command being issued, you can speed it up if you have multiple cores on your CPU using the `-j` option. For example, running `make target -j` would run recipes in parallel on all cores of the CPU at the same time, significantly decreasing build times.

Within the `toolchain/build-binutils/` directory, run the following:
```bash
make
make install
```

You should now have a working version of GNU's Binutils installed at `$PREFIX`.

#### 7.) Configure the GNU Compiler Collection
GCC must be configured, much like Binutils.
```bash
cd Path/to/LensorOS/toolchain
mkdir -p build-gcc
cd build-gcc
../gcc-11.2.0/configure \
    --target=$TARGET \
	--prefix="$PREFIX" \
	--disable-nls \
	--enable-languages=c,c++ \
	--with-sysroot="$SYSROOT"
```

This should generate a Makefile, among other things, that will be used to build GCC in the next step.

Flags:
- `--disable-nls` disables native language support (English-only reduces build size and times).
- `--enable-languages` disables all other languages except for what is stated here (reduces size and build times).
- `--with-sysroot` specifies that GCC can find system headers and libraries at the path specified by the `SYSROOT` variable.

#### 8.) Build the GNU Compiler Collection
Warning: This step takes a long time. Utilize the `-j` option if you have more than a single core CPU.

Within the `toolchain/build-gcc/` directory, run the following:
```bash
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
```

First, we build the new version of GCC (`all-gcc` target).

Next, we build `libgcc` for our target. `libgcc` is a very stripped standard C library that the GCC compiler itself uses. We supply this on our target so that things like fixed width integers, booleans, etc can be used within LensorOS source code.

Finally, we install both the new GCC for our host and `libgcc` for our target. With this complete, you should have a working cross compiler that will generate ELF executables for LensorOS.

If you run into any issues, please let me know/make an issue on GitHub. I'll do my best to help you out.

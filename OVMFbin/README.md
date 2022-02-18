# OVMF
OVMF is a project by TianoCore that targets accurate emulation of real UEFI hardware in virtual environments.

OVMF is part of why this whole project exists; without it LensorOS would need to be tested on real hardware... Suffice to say I'm very grateful.

Due to OVMF depending on the (humongous) EDK2 code-base to create the OVMF images that are required for QEMU to run, 
I provide some pre-built images so everybody that wants to emulate LensorOS doesn't have to build EDK2 + OVMF from source.

If you would prefer to build OVMF anyway, due to trying to target a different architecture or just the ones included being out of date, here are the resources needed to build them yourself:
- [EDK2 Source from GitHub](https://github.com/tianocore/edk2)
  - This includes OVMF, as well as about 30 other packages. We will only be using two of them:
    - `MdeModulePkg`
    - `OvmfPkg`
  - EDK2 has a slightly convoluted build system, but it's nothing too bad if you made it through the cross compiler. I found the following resources immensely helpful for the initial EDK2 build (all links wayback machined for posterity):
    - [EDK2 Common Instructions](https://github.com/tianocore/tianocore.github.io/wiki/Common-instructions)
    - [Using EDK2 with Native GCC](https://github.com/tianocore/tianocore.github.io/wiki/Using-EDK-II-with-Native-GCC)

---

### Building OVMF from Source
##### Warning: This requires over a GiB of hard-drive space.

As with most large builds, utilize the `make -j` flag to allow for more jobs to happen at once (multi-threading).
The general consensus is to pick somewhere around the amount of CPU cores you have.
Whichever you pick, more jobs happening at once means faster builds (and with large ones like the ones upcoming, you will definitely want them to be sped up).

Before you do anything, ensure the following pre-requisites are installed:
```bash
sudo apt install build-essential uuid-dev iasl git gcc-5 nasm python3-distutils
```

To begin, you must obtain the EDK2 source code.
This can be done through [GitHub](https://github.com/tianocore/edk2), [SourceForge](https://sourceforge.net/projects/edk2/), etc.
For this example, I'll show obtaining it from GitHub onto a Unix-like system into a directory `$HOME/EDK2/`.
```bash
cd $HOME
mkdir -p EDK2
cd EDK2
git clone https://github.com/tianocore/edk2.git
cd edk2
git submodule update --init
```

Next, compile the build tools that EDK2 uses to build itself, then setup a basic configuration, then re-build the tools once more :^).
```bash
cd $HOME/EDK2/edk2
make -C BaseTools
. edksetup.sh
make -C BaseTools
```

For EDK2 to work properly, it needs to be told where a few things are installed on the system (namely, it's `BaseTools` directory).
```bash
cd $HOME/EDK2/edk2
export EDK_TOOLS_PATH=$HOME/EDK2/edk2/BaseTools
. edksetup.sh BaseTools
```

Once the build tools have been built and setup correctly, 
configuration files should have been made in the `$HOME/EDK2/edk2/Conf` directory. \
Find the following lines within `target.txt`:
- "TARGET                = DEBUG"
- "TARGET_ARCH           = IA32"
- "TOOL_CHAIN_TAG        = VS2015x86"

Replace them with the following:
- "TARGET                = DEBUG RELEASE"
- "TARGET_ARCH           = X64"
- "TOOL_CHAIN_TAG        = GCC5"

You may be tempted (like me), to try out different versions of GCC, but it turns out `GCC5` is EDK2's way of saying GCC5 *or greater*.

To build `MdeModulePkg`, a dependency of `OvmfPkg`, run the following one-liner:
```bash
cd $HOME/EDK2/edk2 && build -p MdeModulePkg/MdeModulePkg.dsc
```

With `MdeModulePkg` built, we can move on to building `OvmfPkg` to get our `.fd` files necessary for QEMU to boot into a UEFI environment:
```bash
cd $HOME/EDK2/edk2 && build -p OvmfPkg/OvmfPkgX64.dsc
```

A specific size, `x`, of `OVMF.fd` may be defined by appending the following to the above `build` command:
```bash
-DFD_SIZE_IN_KB=x
```

The included OVMF binaries are built using `-DFD_SIZE_IN_KB=2048` for a 2MiB image.

With that, you have successfully built OVMF from source! Congratulations. \
Within the `Build` sub-directory you should be able to find two files within `OvmfX64/FV/`:
- `OVMF_CODE.fd`
- `OVMF_VARS.fd`

These can be copied to `LensorOS/OVMFbin/` and, if correctly renamed, will be used by QEMU to boot from when the `run` batch/shell script is used.
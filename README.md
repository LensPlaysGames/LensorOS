# LensorOS

---

### Table of Contents
- [Booting into LensorOS on hardware](#hardware-boot)
- [Booting into LensorOS using QEMU](#qemu-boot)
- [Booting into LensorOS using VirtualBox](#vbox-boot)
- [Building LensorOS](#build)
  - [A bug regarding CMake's ASM_NASM Makefile generation](#cmake-bug)
- [Acknowledgements](#ack)
  
---

### Booting into LensorOS on hardware <a name="hardware-boot"></a>
DISCLAIMER: LensorOS IS IN NO WAY GUARANTEED TO BE 'SAFE'; RUN AT YOUR OWN RISK! (see [LICENSE](LICENSE))

(pre-compiled binaries coming soon, for now [see the build section](#build))

Ensure you have a USB storage device that is working. Remove all data from the USB. This can be done by formatting, or simply deleting/moving everything off of it.

Next, create a folder called `EFI`, then inside that a folder called `BOOT`. \
Move `main.efi` from `/gnu-efi/x86_64/bootloader/` directory into the `BOOT` folder on the USB. \
Rename `main.efi` in the usb's `BOOT` folder to `bootx64.efi`, as per the UEFI specification.

Finally, navigate back to the root directory of the USB (where the `EFI` folder resides). \
Create a folder. `LensorOS`, then move the following resources into the directory:
- `kernel.elf` from `/kernel/bin/`
- Any `.psf` version 1 font renamed to `dfltfont.psf`
  - A font is included for ease of use and can be found at `kernel/res/dfltfont.psf`.
  - Many free fonts can be found [here](https://github.com/ercanersoy/PSF-Fonts).
  - [GNU Unifont](http://unifoundry.com/unifont/index.html) provides a PSF1 version.

```
Final Directory Structure:
USB
|-- efi
|   `-- boot
|       `-- bootx64.efi
|-- LensorOS
|   |-- kernel.elf
|   `-- dfltfont.psf
`-- startup.nsh
```

---

### Booting into LensorOS using QEMU <a name="qemu-boot"></a>
[Get QEMU](https://www.qemu.org/download)

Before beginning, ensure you have the LensorOS bootloader and kernel binaries. \
(pre-compiled binaries coming soon, for now see [the build section](#build))

To change the font, replace `dfltfont.psf` in the `kernel/res` folder with any PSF1 font (not PSF2). \
For a few fonts that are compatible, check out [this repository](https://github.com/ercanersoy/PSF-Fonts)

Ensure the following dependencies are installed on your system:
- Linux: `sudo apt install mtools`
- [Windows](https://github.com/LensPlaysGames/mtools/releases)

GNU mtools is a set of tools for manipulating MS-DOS style files and filesystems. This is necessary as the UEFI specification requires FAT32 to be used as it's boot device's filesystem.

To build a disk image that will boot into LensorOS, run the following included helper script:
```bash
bash /Path/to/LensorOS/kernel/mkimg.sh
```

Upon completion, this will have generated a disk image file `LensorOS.img` that follows the UEFI standards, meaning any UEFI-supporting machine could boot from this image, given it is a valid boot device.

- [Continue on Linux](#qemu-boot-linux)
- [Continue on Windows](#qemu-boot-windows)

#### On Linux <a name="qemu-boot-linux"></a>
To run QEMU with the correct command line options automatically, use the following helper script to launch QEMU booting into LensorOS:
```bash
bash /Path/to/LensorOS/kernel/run.sh
```
For debugging with gdb, run `bash rundbg.sh` instead. This will launch QEMU but wait to start cpu execution until `gdb` has connected on port `1234` of `localhost`. \
See [a note about debugging](#gdb-debug-note-1).

#### On Windows <a name="qemu-boot-windows"></a>
To launch QEMU from the generated disk image, A `run.bat` file is included. \
The batch file requires the directory that the QEMU executable resides in be added to the system's PATH variable. [See this stackoverflow thread for help](https://stackoverflow.com/questions/9546324/adding-a-directory-to-the-path-environment-variable-in-windows). \
If editing the PATH variable isn't working, the batch script could always be edited to use the exact path to the QEMU executable on your local machine. \

By simply double clicking this batch file, QEMU will open and boot into LensorOS. 

All serial output will be re-directed to stdin/stdout, which is likely a `cmd.exe` window that opened along with QEMU. The problem with `cmd.exe` is that it leaves the output looking rather mangled (ie. left-pointing arrows, open square-brackets, etc). This is due to the terminal not supporting ANSI color codes.

To get around this, I use the (rather life-changing) [Windows Terminal](https://github.com/Microsoft/Terminal). It should be the default terminal, and should have been for five years now, but I'm glad it is available and open source none-the-less.

This new terminal allows both WSL and PowerShell to be open in the same terminal, but separate tabs. By the `run.bat` file from within a PowerShell in the new Windows Terminal, you will experience glorious full-color, formatted serial output.

If you are having none of this, and would prefer to have a very monotone serial output that is also not mangled with ANSI color codes, define `LENSOR_OS_UART_HIDE_COLOR_CODES` during compilation and all color codes will be hidden from serial output by the kernel itself.

There is also a `rundbg.bat` that will launch QEMU with the appropriate flags to wait for `gdb` to connect on port `1234` of `localhost`. \
<a name="gdb-debug-note-1"></a>NOTE: When debugging with gdb, the kernel must be built with debug symbols ("-g" compile flag). To achieve this, run cmake with the following command line argument: \
`-DCMAKE_BUILD_TYPE=Debug`

---

### Booting into LensorOS using VirtualBox <a name="vbox-boot"></a>
[Get VirtualBox](https://www.virtualbox.org/wiki/Downloads)

VirtualBox is kind of picky in the file formats it will accept as drives and such. \
Because of this fact, we must prepare an `.iso` file with an ISO filesystem to boot into VirtualBox.

Before beginning, ensure you have the LensorOS bootloader and kernel binaries built. \
(pre-compiled binaries coming soon, for now see [the build section](#build))

To change the font, replace `dfltfont.psf` in the `kernel/res` folder with any PSF1 font (not PSF2). \
For a few fonts that are compatible, check out [this repository](https://github.com/ercanersoy/PSF-Fonts)

NOTE: On Windows, complete the following shell commands from within WSL, or the Windows Subsystem for Linux.

Install the tool necessary to create `.iso` files and ISO-9660 filesystems, as well as a tool to create and format MS-DOS style filesystems:
```bash
sudo apt install mtools xorriso
```
Next, simply run the `mkiso.sh` script with Bash:
```bash
bash /Path/to/LensorOS/kernel/mkiso.sh
```

If all goes well, this will first generate a FAT32 EFI-compatible boot disk image, then create a bootable ISO-9660 CD-ROM disk image. \
To actually use this boot cd in a VM in VirtualBox, it requires some setup:
1. Open VirtualBox.
2. Click the `New` button.
3. Give the VM a name and a file path you are comfortable with.
4. Select Type of `Other` and Version of `Other/Unknown (64-bit)`.
5. Leave the memory size how it is; 64MB is plenty at this time.
6. Select the `Do not add a virtual hard disk` option.
7. Click the `Create` button to create the new virtual machine.
8. With the new VM selected within the list on the left, click the `Settings` button.
9. Navigate to `System` within the list on the left.
    1. Change Chipset to `ICH9`.
    2. Enable Extended Feature `Enable EFI (special OSes only)`.
    3. Navigate to the `Processor` tab, and check the `Enable Nested VT-x/AMD-V` checkbox.
10. Navigate to `Storage` within the list on the left.
    1. Right click the storage controller (default IDE), and select `Optical Drive`.
    2. Click the `Add` button in the new `Optical Disk Selector` window that pops up.
    3. Browse to `Path/To/LensorOS/kernel/bin/` and select `LensorOS.iso`.
    4. Ensure `LensorOS.iso` is selected within the list, and click the `Choose` button.
11. Navigate to `Network` within the list on the left.
    1. Disable all network adapters.

After all of this has been done, you are ready to click `Start` on the VirtualBox VM; the bootloader should run automatically.

---

### Building LensorOS <a name="build"></a>
On Windows, some way of accessing a linux command-line environment is necessary. \
Personally, I use `WSL` with the `Ubuntu 20.04` distro. \
Alternatively, one could use a virtual machine that emulates a linux distro (ie. `VirtualBox` running `Linux Mint`, or something), and develop from there. \
There's also things like `Cygwin`, if you want to stay on Windows 100% of the time.

#### 1.) Obtain the source code
To begin, clone this repository to your local machine (or fork it then clone it, it's up to you if you'd like to make your own version, or contribute to this one through pull requests).

Example clone command:
```bash
git clone https://github.com/LensPlaysGames/LensorOS
```

Open a Linux terminal, then `cd` to the root directory of the repository.

NOTE: If on Windows using WSL, I recommend cloning onto your Windows machine partition. If you use the WSL partition to store LensorOS, many tools can not access the fake network path that is used (ie. `\\wsl$\`), and it over-all just causes a head-ache.

#### 2.) Build the bootloader
Ensure that you have previously ran `sudo apt install build-essential mtools` to get the necessary compilation pre-requisites.

First, `cd` to the `gnu-efi` directory, and run the following:
```bash
make
make bootloader
```

NOTE: One only needs to run `make` for the bootloader once. \
Following that, simply using the `bootloader` target will be sufficient to update the bootloader.

#### 3.) Build the kernel
Before compiling the kernel, you must build the toolchain that LensorOS uses. \
See [README.toolchain](kernel/README.toolchain.md) within the `/kernel/` directory of the repository for explicit build instructions.

Parts of the kernel are written in assembly intended for the Netwide Assembler. \
Ensure it is installed on your system before continuing:
- Linux: `sudo apt install nasm`
- [Windows](https://nasm.us/)

Once the toolchain is up and running (added to `$PATH` and everything), continue the following steps.

NOTE: It is possible to use your host compiler to build the kernel. It is not recommended, and likely won't work, but who am I to stop you. Simply remove/comment out the `set(CMAKE_C_COMPILER` and `set(CMAKE_CXX_COMPILER` lines within [CMakeLists.txt](kernel/CMakeLists.txt). This will instruct CMake to use your host machine's default compiler (again, **not** recommended).

First, `cd` to the `kernel` directory of the repository. \
To prepare a build system that will build the kernel with `GNU make`, run the following:
```bash
cmake -S . -B out
cd out
cmake .
```

At this point everything should be set up to build the kernel. \
To do that, invoke the build system that was generated by CMake (default `make`). \
Alternatively, generate any build system of your choice that is supported by CMake (I recommend Ninja, it's fast).

This final build step will generate `kernel.elf` within the `/kernel/bin` directory, ready to be used in a boot usb or formatted into an image.

If building for real hardware, ensure to remove all virtual machine definitions (ie. `QEMU`, `VBOX`) from [CMakeLists.txt](kernel/CMakeLists.txt). \
This allows for the hardware timers to be used to their full potential (asking QEMU for 1000hz interrupts from multiple devices overloads the emulator and guest time falls behind drastically; to counter-act this, very slow frequency periodic interrupts are setup as to allow the emulator to process them accordingly, allowing for accurate time-keeping in QEMU).

If you are familiar with CMake, this sequence might look *slightly* strange to you (namely the second `cmake` command). \
See the [CMake bug](#cmake-bug) section for more details on why it is needed, and maybe you have an even better fix.

#### A bug regarding CMake's ASM_NASM Makefile generation <a name="cmake-bug"></a>

It may very well be that the bug is within the user error domain. If that is the case, I'll be thoroughly embarassed yet glad to fix my mistake.

The reason for re-building the CMake-generated build system directly after doing it for the first time is to fix a bug that appears to be within the ASM_NASM CMake Makefile generation. \
This bug causes any objects built with NASM to be 32-bit (no bueno for our 64-bit project). \
I've found that, for some reason, re-building the system right away like this fixes the issue.
If you would like to look into this, pay attention to `CMakeFiles/Assembly.dir/build.make`; how it appears before the `cmake .` command, and how it appears after. \
One way this can be achieved is using the common linux command: `diff`. \
It simply displays any differences between two given files, displaying nothing if they are the same.

My `diff` output on the before and after (I saved a copy of the generated `build.make` as `build_.make`):
```
lensor-radii@Garry:~/LensorOS/kernel/out$ diff CMakeFiles/Assembly.dir/build.make CMakeFiles/Assembly.dir/build_.make
lensor-radii@Garry:~/LensorOS/kernel/out$ cmake .
-- Configuring done
-- Generating done
-- Build files have been written to: /home/lensor-radii/LensorOS/kernel/out
lensor-radii@Garry:~/LensorOS/kernel/out$ diff CMakeFiles/Assembly.dir/build.make CMakeFiles/Assembly.dir/build_.make
63c63
<       /usr/bin/nasm $(ASM_NASM_INCLUDES) $(ASM_NASM_FLAGS) -f elf64 -o CMakeFiles/Assembly.dir/src/gdt.asm.o /home/lensor-radii/LensorOS/kernel/src/gdt.asm
---
>       /usr/bin/nasm $(ASM_NASM_INCLUDES) $(ASM_NASM_FLAGS) -f elf -o CMakeFiles/Assembly.dir/src/gdt.asm.o /home/lensor-radii/LensorOS/kernel/src/gdt.asm
lensor-radii@Garry:~/LensorOS/kernel/out$
```

As you can see, the file `build.make` has been changed by the `cmake .` command, altering it to actually use the flags I pass in `kernel/CMakeLists.txt` with the `target_compile_options()` CMake function. \
I don't know why the re-build is necessary, and it takes very little time, but if you know anything on why this occurs I would greatly appreciate you letting me know.

### Acknowledgements <a name="ack"></a>
At first, development followed tutorials that can be found at [Poncho's GitHub](https://github.com/Absurdponcho). \
Those tutorials were abandoned just after setting up a very basic AHCI driver, so I've taken the wheels from there.

A large thanks to the huge portions of inspiration, knowledge, and help that came from the OSDev [wiki](https://wiki.osdev.org/Expanded_Main_Page) and [forums](https://forum.osdev.org/) (sorry, Terry).

A huge amount of entertainment and inspiration has come from [SerenityOS](https://github.com/SerenityOS/serenity), an operating system being built by the OS development hobby-ist community.

The compeletely-from-scratch [ToaruOS](https://github.com/klange/toaruos) has also been an amazing source of knowledge, inspiration, and information.

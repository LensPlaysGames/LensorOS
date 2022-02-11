# LensorOS
At first, development followed tutorials that can be found at [Poncho's GitHub](https://github.com/Absurdponcho). \
Those tutorials were abandoned just after setting up a very basic AHCI driver, so I've taken the wheels from there.

A large thanks to the huge portions of inspiration, knowledge, and help that came from the OSDev [wiki](https://wiki.osdev.org/Expanded_Main_Page) and [forums](https://forum.osdev.org/).

---

### Table of Contents
- [Booting into LensorOS on hardware](#hardware-boot)
- [Booting into LensorOS using QEMU](#qemu-boot)
- [Building LensorOS](#build)
  - [A bug regarding CMake's ASM_NASM Makefile generation](#cmake-bug)
  
---

### Booting into LensorOS on hardware <a name="hardware-boot"></a>
###### DISCLAIMER: LensorOS IS IN NO WAY GUARANTEED TO BE 'SAFE'; RUN AT YOUR OWN RISK! (see LICENSE)

(pre-compiled binaries coming soon, for now see [the build section](#build))

Ensure you have a USB storage device that is working. Remove all data from the USB. This can be done by formatting, or simply deleting/moving everything off of it.

Next, create a folder called `efi`, then inside that a folder called `boot`. \
Move `main.efi` from `/gnu-efi/x86_64/bootloader/` directory into the `boot` folder on the USB. \
Rename `main.efi` in the usb's `boot` folder to `bootx64.efi`.

Finally, navigate back to the root directory of the USB (where the `efi` folder resides). \
Create a folder. `LensorOS`, then move the following resources into the directory: \
- `kernel.elf` from `/kernel/bin/`
- Any `.psf` version 1 font renamed to `dfltfont.psf`
  - A font is included for ease of use and can be found at `kernel/res/dfltfont.psf`.
  - Many free fonts can be found [here](https://github.com/ercanersoy/PSF-Fonts)

```
Final Directory Structure:
USB
|\
| efi
|  \
|   boot
|    \
|     bootx64.efi
|\
| LensorOS
| |\
| | kernel.elf
|  \
|   dfltfont.psf
 \
  startup.nsh
```

---

### Booting into LensorOS using QEMU <a name="qemu-boot"></a>
(pre-compiled binaries coming soon, for now see [the build section](#build))

To change the font, replace `dfltfont.psf` in the `kernel/res` folder with any PSF1 font (not PSF2). \
For a few fonts that are compatible, check out [this repository](https://github.com/ercanersoy/PSF-Fonts)

#### On Linux

`bash mkimg.sh` will generate a `.iso` disk image file that can be used as a boot drive by a virtual machine that supports OVMF like [QEMU](https://www.qemu.org/).

`bash run.sh` will boot up QEMU into LensorOS. \
QEMU does need to be installed, so make sure you first run (`sudo apt install qemu-system-x86`).

See [a note about debugging](#gdb-debug-note-1).

For debugging with gdb, run `bash rundbg.sh` instead. This will launch QEMU but wait to start cpu execution until `gdb` has connected on port `1234` of `localhost`.

#### On Windows

In a linux terminal (WSL, Cygwin, etc), run `bash /path/to/LensorOS/kernel/mkimg.sh` to generate a disk image file that will be used by QEMU as a boot drive.

To launch QEMU from the generated disk image, A `run.bat` file is included. \
By simply double clicking this, QEMU will run, booting into a UEFI environment that will load the LensorOS bootloader. The batch file requires the directory that the QEMU executable resides in be added to the system's PATH variable. [See this stackoverflow thread for help](https://stackoverflow.com/questions/9546324/adding-a-directory-to-the-path-environment-variable-in-windows). \
If editing the PATH variable isn't working, the batch script could always be edited to use the exact path to the QEMU executable on your local machine.

If you're terminal output looks rather mangled (ie. left-pointing arrows, open square-brackets, etc), it is likely the terminal you are using doesn't support ANSI color codes. \
This is how it is when I use Windows Explorer to launch the `run` batch script, and makes debugging using the serial output rather difficult. \
To get around this, I use the (rather life-changing) [Windows Terminal](https://github.com/Microsoft/Terminal). It should be the default terminal, and should have been for five years now, but I'm glad it is available and open source none-the-less. \
This new terminal allows both WSL and PowerShell to be open in the same terminal, but separate tabs. By running `& '\\wsl$\your-linux-distro\path\to\LensorOS\kernel\run.bat'` from within a PowerShell in the new Windows Terminal, you will experience glorious full-color, formatted serial output. If you are having none of this, and would prefer to have a very monotone serial output that is also not mangled with ANSI color codes, define `LENSOR_OS_UART_HIDE_COLOR_CODES` and all color codes will be hidden from serial output.

There is also a `rundbg.bat` that will launch QEMU with the appropriate flags to wait for `gdb` to connect on port `1234` of `localhost`.

<a name="gdb-debug-note-1"></a>NOTE: When debugging with gdb, the kernel must be built with debug symbols ("-g" compile flag). To achieve this, run cmake with the following definition: \
`-DCMAKE_BUILD_TYPE=Debug`

---

### Building LensorOS <a name="build"></a>
To begin, clone this repository to your local machine (or fork it then clone it, it's up to you if you'd like to make your own version, or contribute to this one through pull requests).

Example clone command: \
`git clone https://github.com/LensPlaysGames/LensorOS`

On Windows, some way of accessing a linux command-line environment is necessary. \
Personally, I use `WSL` with the `Ubuntu 20.04` distro. \
Alternatively, one could use a virtual machine that emulates a linux distro (ie. `VirtualBox` running `Linux Mint`, or something), and develop from there. \
There's also things like `Cygwin`, if you want to stay on Windows 100% of the time.

Open a Linux terminal, then `cd` to the root directory of the repository.

Ensure that you have previously ran `sudo apt install build-essential mtools` to get all necessary compilation tools.

First, `cd` to the `gnu-efi` directory, and run the following: \
`make` \
`make bootloader`

NOTE: One only needs to run `make` for the bootloader once. \
Following that, simply using the `bootloader` target will be sufficient to build the bootloader.

Next, `cd` to the `kernel` directory of the repository.

To build the kernel, run the following: \
`cmake -S . -B out` \
`cd out` \
`cmake .` \
`make`

This final step will generate `kernel.elf` within the `kernel/bin` directory, ready to be used in a boot usb or formatted into an image.

If you are familiar with CMake, this might look *slightly* strange to you (namely the second `cmake` command). \
See the [CMake bug](#cmake-bug) section for more details on why it is needed, and maybe you have an even better fix.

If building for real hardware, ensure to remove `-DQEMU` from CMakeLists.txt. \
This allows for the hardware timers to be used to their full potential (asking QEMU for 1000hz interrupts from multiple devices overloads the emulator and guest time falls behind drastically; to counter-act this, very slow frequency periodic interrupts are setup as to allow the emulator to process them accordingly, allowing for accurate time-keeping even in QEMU).

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

# LensorOS
A 64-bit operating system with everything built from scratch!

---

### Table of Contents
- [Booting into LensorOS on hardware](#hardware-boot)
- [Booting into LensorOS using QEMU](#qemu-boot)
- [Booting into LensorOS using VirtualBox](#vbox-boot)
- [Building LensorOS](#build)
- [Acknowledgements](#ack)

---

## Booting into LensorOS on hardware <a name="hardware-boot"></a>
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

## Booting into LensorOS using QEMU <a name="qemu-boot"></a>
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

Alternatively, Create a disk image file that is GPT formatted (aka supports partitions) by utilizing the `mkgpt.sh` script that is included. [See the scripts README](kernel/scripts/README.scripts.md) for more information.

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

## Booting into LensorOS using VirtualBox <a name="vbox-boot"></a>
[Get VirtualBox](https://www.virtualbox.org/wiki/Downloads)

VirtualBox is kind of picky in the file formats it will accept as drives and such. \
Because of this fact, VirtualBox can not use the `.img` file raw like QEMU, and must prepare either a `.bin` GPT formatted disk image with an EFI System partition or `.iso` file with an ISO filesystem.

Before beginning, ensure you have the LensorOS bootloader and kernel binaries built. \
(pre-compiled binaries coming soon, for now see [the build section](#build))

To change the font, replace `dfltfont.psf` in the `kernel/res` folder with any PSF1 font (not PSF2). \
For a few fonts that are compatible, check out [this repository](https://github.com/ercanersoy/PSF-Fonts), or even [GNU Unifont](http://unifoundry.com/unifont/index.html).

There are two possible pathways that LensorOS bootable media can be generated.
- [GPT](#vbox-boot-gpt)
- [Live CD](#vbox-boot-live-cd)

#### GPT <a name="vbox-boot-gpt"></a>
Utilize the `mkgpt.sh` script that is included in the `kernel/scripts/` directory. [See the scripts README](kernel/scripts/README.scripts.md) for more information on this script.

If all has went well, there will be a `LensorOS.img` and a `LensorOS.bin` in the `/kernel/bin/` directory of the repository.
While this binary `.bin` file is a perfectly valid image of a real GPT formatted disk, VirtualBox does not accept it as valid.
When VirtualBox is installed, it also installs a lot of command line tools, one of which is called `VBoxManage`.
This tool has a subcommand `convertfromraw` that we will utilize to create a `.vdi` virtual disk image from our `.bin` binary disk image.

```bash
VBoxManage convertfromraw /Path/to/LensorOS/kernel/bin/LensorOS.bin /Path/to/LensorOS/kernel/bin/LensorOS.vdi --format VDI
```

Continue to the [VirtualBox VM Configuration](#vbox-boot-vm-config), and replace any mention of `Optical Drive` with `Hard Disk`, and any mention of `LensorOS.iso` with `LensorOS.vdi`.

#### Live CD <a name="vbox-boot-live-cd"></a>
NOTE: On Windows, complete the following shell commands from within WSL, or the Windows Subsystem for Linux.

Install the tool necessary to create `.iso` files and ISO-9660 filesystems, as well as a tool to create and format MS-DOS style filesystems:
```bash
sudo apt install mtools xorriso
```
Next, simply run the `mkiso.sh` script with Bash:
```bash
bash /Path/to/LensorOS/kernel/mkiso.sh
```

If all goes well, this will first generate a FAT32 EFI-compatible boot disk image, then create a bootable ISO-9660 CD-ROM disk image.

#### Virtual Machine Configuration <a name="vbox-boot-vm-config"></a>
To actually use the generated bootable media in a VM in VirtualBox, it requires some setup:
1. Open VirtualBox.
2. Click the `New` button to create a new virtual machine (VM).
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
    4. Ensure the image just added is selected within the list, and click the `Choose` button.
11. Navigate to `Network` within the list on the left.
    1. Disable all network adapters.

After all of this has been done, you are ready to click `Start` on the VirtualBox VM; the bootloader should run automatically.

---

## Building LensorOS <a name="build"></a>
There are three steps to building LensorOS:
1. [the Toolchain](toolchain/README.md)
1. the Bootloader
2. the Kernel

On Windows, use the Windows Subsystem for Linux (WSL) to emulate Linux exactly, no drawbacks. \
If you don't want to use WSL, you'll need pre-built binaries of a [Canadian Cross](https://wiki.osdev.org/Canadian_Cross) LensorOS Toolchain, the bootloader, and [GNU mtools](https://github.com/LensPlaysGames/mtools/releases); ask and we will help you on your journey :^). \
Keep in mind the bootloader can't be built natively on Windows (yet), so skip step #2 if not using WSL.

Alternatively, one could use a virtual machine that emulates a linux distro (ie. `VirtualBox` running `Linux Mint`, or something), and develop from there.

There's also [Cygwin](https://www.cygwin.com/) for a Unix-like environment on native Windows, but I am not knowledgable on this topic.

### Linux <a name="build-linux"></a>
If you are on Windows 10+, you are able to use WSL to complete the following Linux steps as-is.

Get dependencies:
```bash
sudo apt install build-essential cmake git make nasm
```

Obtain the source code:
```bash
git clone https://github.com/LensPlaysGames/LensorOS
```
NOTE: If on Windows using WSL, I recommend cloning onto your Windows
  machine partition (i.e. a path starting with `/mnt/c/` or something).
If you use the WSL partition to store LensorOS, many tools can not access the fake
  network path that is used (ie. `\\wsl$\`), and it over-all just causes a head-ache.

#### 1. Build The LensorOS Toolchain
[Follow the instructions in the toolchain README](toolchain/README.md)

#### 2. Build the bootloader
The bootloader source code resides in the `gnu-efi` directory, for now.
```bash
cd /Path/to/LensorOS/gnu-efi/
make
make bootloader
```
This will generate `main.efi` in the `/gnu-efi/x86_64/bootloader/` directory within the repository.
This is the UEFI compatible executable file.

NOTE: One only needs to run `make` for the bootloader once; it generates `libgnuefi.a`, which the bootloader itself links with. Following that, simply using the `bootloader` target will be sufficient to update the bootloader; even that only needs to be done if the bootloader code was changed.

#### 3. Build the kernel
The LensorOS Kernel uses CMake to generate the build system; beware, as not all the build systems CMake can generate honor the request to use the LensorOS Toolchain (*ahem* Visual Studio *ahem*).
To prepare a build system that will build the kernel with `GNU make`, run the following:
```bash
cd /Path/to/LensorOS/kernel/
cmake -S . -B rls -DCMAKE_BUILD_TYPE=Release
```

The above command should generate an out of source build system
  in the `rls` subdirectory that is the default for your host. \
To build the kernel, invoke the build system that was
  generated by CMake; by default it's GNU's `make`. \
Alternatively, generate any build system of your choice that
  is supported by CMake (I recommend Ninja, it's fast).
No matter the build system you choose, invoke it using the following command:
```bash
cmake --build /Path/to/LensorOS/kernel/rls
```
NOTE: It's not necessary to provide an absolute path, but it
  means the command can be run from any working directory.

This final build step will generate `kernel.elf` within the `/kernel/bin` directory,
  ready to be used in a boot usb or formatted into an image. \
See the sections starting with "Booting into LensorOS" above.

If building the kernel for real hardware, ensure to set the `MACHINE` [kernel CMakeLists.txt](kernel/CMakeLists.txt) variable to `PC`, and not any of the virtual machines (ie. `QEMU`, `VBOX`).
This allows for the hardware timers to be used to their full potential (asking QEMU for 1000hz interrupts from multiple devices overloads the emulator and guest time falls behind drastically; to counter-act this, very slow frequency periodic interrupts are setup as to allow the emulator to process them accordingly, allowing for accurate time-keeping in QEMU).

---

### Acknowledgements <a name="ack"></a>
At first, development followed tutorials that can be found at [Poncho's GitHub](https://github.com/Absurdponcho). \
Those tutorials were abandoned just after setting up a very basic AHCI driver, so I've taken the wheels from there.

A large thanks to the huge portions of inspiration, knowledge, and help that came from the OSDev [wiki](https://wiki.osdev.org/Expanded_Main_Page) and [forums](https://forum.osdev.org/) (sorry, Terry).

A huge amount of entertainment and inspiration has come from [SerenityOS](https://github.com/SerenityOS/serenity), an operating system being built by the OS development hobbyist community.

The compeletely-from-scratch [ToaruOS](https://github.com/klange/toaruos) has also been an amazing source of knowledge, inspiration, and information.

#### LensorOS Birthday
My birthday is January 14th! I am still a *wittle baby*.

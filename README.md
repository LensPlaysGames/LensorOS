# LensorOS
At first, development followed tutorials that can be found at [Poncho's GitHub](https://github.com/Absurdponcho). \
Those tutorials were abandoned just after setting up a very basic AHCI driver, so I've taken the wheels from there.

A large thanks to the huge portions of inspiration, knowledge, and help that came from the OSDev [wiki](https://wiki.osdev.org/Expanded_Main_Page) and [forums](https://forum.osdev.org/).

### Running LensorOS on hardware
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
  - A font is included for ease of use: `zap-vga16.psf`. 
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

### Running LensorOS on an emulator
(pre-compiled binaries coming soon, for now see [the build section](#build))

First, you must copy the `.psf` font (MUST be PSF1, not PSF2) into the `/kernel/bin/` directory of the repository. \
Next, rename the font file to `dfltfont.psf`.

It takes just one command to generate a disk image that is bootable from something like QEMU: \
`make buildimg`

This will generate a `.iso` image file that can be used as a boot disk in a virtual machine like [QEMU](https://www.qemu.org/).

If on Windows, a `run.bat` file is included. Simply double click this to run QEMU, booting directly from the LensorOS bootloader. The batch file requires the directory the QEMU executable resides in be added to the system's PATH variable [see this stackoverflow thread for help](https://stackoverflow.com/questions/9546324/adding-a-directory-to-the-path-environment-variable-in-windows).

If on Linux, run `make run` and QEMU should boot up into LensorOS. \
QEMU does need to be installed, so make sure you first run (`sudo apt install qemu`).

### Building LensorOS <a name="build"></a>
On Windows, `WSL` is required. I use `Ubuntu 20.04` distro. \

Open a Linux terminal, then `cd` to the root directory of the repository.

Ensure that you have previously ran `sudo apt install build-essentials mtools` to get all necessary compilation tools.

To initialize the directories needed, `cd` to the `kernel` folder and run the following: \
`make setup`

<a name="bootloader-step"></a>
After this, `cd` to the `gnu-efi` folder and run the following: \
`make bootloader`

To complete the build, run: \
`cd ../kernel` \
`make kernel`

This will generate a `.efi` file from the kernel source code. 

When re-building (ie. after changes are made to the source code), simply start at the [bootloader step](#bootloader-step) and go from there to re-compile, then follow the corresponding 'Running LensorOS' section for use in a virtual machine or on real hardware.

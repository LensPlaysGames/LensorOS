# LensorOS Helper Scripts
This file outlines the dependencies, usage, and functionality of the scripts within this directory.

### Table of Contents:
- [mkimg.sh](#mkimg-sh)
- [mkiso.sh](#mkiso-sh)
- [startup.nsh](#startup-nsh)
- [visualize_includes.sh](#visualize-includes-sh)

---

### `mkimg.sh` <a name="mkimg-sh"></a>
Generate a UEFI-compatible bootable disk image file with everything necessary to boot LensorOS loaded onto it (namely the bootloader and kernel binaries, but also resources like the font).

The disk image generated may be formatted onto an actual USB drive using a tool like `dd`, or it may be used by a virtual machine that supports (U)EFI to boot from directly, like QEMU.

NOTE: The bootloader and kernel binaries **must** be built prior to running this script (see build instructions in the README within the source directory of the repository).

Invocation:
```bash
bash mkimg.sh
```

Dependencies:
- GNU MTools
  - For pre-built releases, see the [releases section of my fork of GNU MTools on GitHub](https://github.com/LensPlaysGames/mtools/releases)
  - On Debian distributions: `sudo apt install mtools`
  - [Source Code](http://ftp.gnu.org/gnu/mtools/mtools-4.0.38.tar.gz)
    - There are great instructions and scripts included to help with generating executables for a variety of platforms. On Windows, you may need to utilize WSL to bootstrap the Windows executable counter-part of MTools.
- `dd`
  - Default with GNU/Linux
  - On Windows, **any *one* of the following**: 
    - [WSL](https://docs.microsoft.com/en-us/windows/wsl/about)
	- [MinGW](https://sourceforge.net/projects/mingw/)
	- [Cygwin](https://www.cygwin.com/)
	
---

### `mkiso.sh`  <a name="mkiso-sh"></a>
Generate a bootable ISO-9660 filesystem in the "El Torito" configuration. This image could be burned to an actual disk and booted from as a live CD, or attached to a virtual machine in VirtualBox as outlined in the README in the root directory of the repository.

NOTE: This script runs `mkimg.sh` to generate the FAT32 image, then generates the `.iso` file from that image.

Invocation:
```bash
bash mkiso.sh
```

Dependencies:
- [GNU xorriso](https://www.gnu.org/software/xorriso/)
  - On Debian distributions: `sudo apt install xorriso`
  - Pre-built Windows executables can be found [in this repository](https://github.com/PeyTy/xorriso-exe-for-windows)
  - [Source Code](https://www.gnu.org/software/xorriso/xorriso-1.5.4.pl02.tar.gz)

---

### `startup.nsh` <a name="startup-nsh"></a>
This is a file used by `mkimg.sh` to ensure the bootloader is ran when the computer boots from the generated image.

---

### `visualize_includes.sh` <a name="visualize-includes-sh"></a>
Generate visual representations (PNG images) of the include dependencies present within the LensorOS kernel. This script may take some time to complete (>1min). It works by downloading a Perl script `cinclude2dot.pl`, and running that to generate the dependency data in a format Graphviz understands: `.dot`. With this `.dot` file, it then generates PNG images using two different visual filters in Graphviz: one for a directed graph and one for an undirected graph. All of the generated files may be found in the `/kernel/include_visualization` directory.

Invocation:
```bash
bash visualize_includes.sh [OPTIONS]
```
Square brackets denote a field is optional.

Options:
- `-i <paths>`
  - If headers are not being detected by the Perl script, add the path(s) to them with this option. For the LensorOS kernel, this mostly means a path to the compiler's library include directory. `paths` is a comma separated list of paths to directories.

Dependencies:
- [Curl](https://curl.se/)
  - Default on most systems (Windows, GNU/Linux)
- [Perl](https://www.perl.org/)
  - Default on most systems (Windows, GNU/Linux)
- [Dot/SFDP](https://www.graphviz.org/download/)

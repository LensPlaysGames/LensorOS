# LensorOS Scripts
A collection of shell scripts that automate processes surrounding LensorOS.

NOTE: All of the given shell commands expect the
  current working directory to be this directory.

Regarding PowerShell `.ps1` scripts:
  Running scripts is not allowed with the default ExecutionPolicy.
  Use `Set-ExecutionPolicy Bypass -Scope Process` to be able to
  run scripts for the lifetime of the current PowerShell process.

---

### Table of Contents:
- Visualization
  - [cinclude2dot.sh](#kernel-cinclude2dot-sh)
- Boot Media Generation
  - Bash
    - [sysroot.sh](#bootmediagen-sysroot-sh)
    - [mkbootdir.sh](#bootmediagen-mkbootdir-sh)
    - [install_mkgpt.sh](#bootmediagen-installmkgpt-sh)
    - [mkgpt.sh](#bootmediagen-mkgpt-sh)
    - [mkgpt_fdisk.sh](#bootmediagen-mkgptfdisk-sh)
    - [mkimg.sh](#bootmediagen-mkimg-sh)
    - [mkiso.sh](#bootmediagen-mkiso-sh)
  - Batch
    - [mkimg.bat](#bootmediagen-mkimg-bat)
    - [mkiso.bat](#bootmediagen-mkiso-bat)
  - PowerShell
    - [mkimg.ps1](#bootmediagen-mkimg-ps1)
    - [mkiso.ps1](#bootmediagen-mkiso-ps1)
- Booting into LensorOS
  - Bash
    - [run.sh](#booting-run-sh)
    - [runhda.sh](#booting-runhda-sh)
    - [runiso.sh](#booting-runiso-sh)
  - Batch
    - [run.bat](#booting-run-bat)
    - [runhda.bat](#booting-runhda-bat)
    - [runiso.bat](#booting-runiso-bat)
  - PowerShell
    - [run.ps1](#booting-run-ps1)
    - [runhda.ps1](#booting-runhda-ps1)
    - [runiso.ps1](#booting-runiso-ps1)

---

### `cinclude2dot.sh` <a name="kernel-cinclude2dot-sh"></a>
`cinclude2dot.pl` is a perl script that converts
  `C` style includes into the DOT file format.
  The DOT file format can be graphed by a tool
  called graphviz in a multitude of ways.

This bash script downloads that perl script
  if it isn't already, then runs it, then runs
  graphviz to generate two `png`s, each with
  differing representations of the same data.
  An `include_visualization` directory will be
  generated in the root of the repository with
  the downloaded script and generated images.

Invocation:
```bash
bash cinclude2dot.sh [-i comma-separated-include-paths]
```
NOTE: Anything inside square brackets `[]` is optional.

Dependencies:
- [Curl](https://curl.se/download.html)
  - This is likely already on your system.
- [Perl](https://www.perl.org/get.html)
  - This is likely already on your system.
- [GraphViz](https://www.graphviz.org/download/)

---

### `sysroot.sh` <a name="bootmediagen-sysroot-sh"></a>
Generate a system root for LensorOS and the LensorOS
  toolchain in the `/root/` directory of the repository.

Invocation:
```bash
bash sysroot.sh
```

No Dependencies.

---

### `mkbootdir.sh` <a name="bootmediagen-mkbootdir-sh"></a>
Create a bootable directory structure in the `boot` subdirectory.

This boot directory could be flashed onto a physical USB storage media stick
  and (theoretically) booted from like a floppy disk on any UEFI compatible machine.

Invocation:
```bash
bash mkbootdir.sh
```

No Dependencies.

---

### `install_mkgpt.sh` <a name="bootmediagen-installmkgpt-sh"></a>
`mkgpt` is a command line tool that can generate disk
  images with a valid GUID Partition Table, or GPT.
  It is utilized to generate bootable media for LensorOS.

Sadly, there aren't many pre-built binaries available.

Luckily, it's very easy to clone and build yourself,
  thanks to this bash script. It does everything for you,
  from downloading dependencies to installing the program.

NOTE: This script requires super user privileges to
  automatically install dependencies as well as to
  install the program `mkgpt` when it is done building.
  I recommend looking at the script code before you run
  it to ensure you are okay with what you are about to run.

Invocation:
```bash
bash install_mkgpt.sh
```

Dependencies:
- [GNU autoconf](https://www.gnu.org/software/autoconf/)
- [GNU automake](https://www.gnu.org/software/automake/)

---

### `mkgpt.sh` <a name="bootmediagen-mkgpt-sh"></a>
Generate a bootable disk image with a valid GUID Partition Table.
  An `EFI System` partition is added with the contents of `LensorOS.img`.

[mkimg.sh](#bootmediagen-mkimg-sh) is run before generating the `EFI System`
  partition to ensure the boot media is as up to date as possible.

Invocation:
```
bash mkgpt.sh
```

Dependencies:
- [mkgpt](#bootmediagen-install-mkgpt-sh)

---

### `mkgpt_fdisk.sh` <a name="bootmediagen-mkgpt-sh"></a>
Generate a bootable disk image with a valid GUID Partition Table.
  An `EFI System` partition is added with the contents of `LensorOS.img`.

[mkimg.sh](#bootmediagen-mkimg-sh) is run before generating the `EFI System`
  partition to ensure the boot media is as up to date as possible.

Invocation:
```
bash mkgpt_fdisk.sh
```

Dependencies:
- fdisk -- Native command on Unix

---

### `mkimg.sh` <a name="bootmediagen-mkimg-sh"></a>
Generate a bootable disk image that is UEFI compatible (FAT32 formatted).

Invocation:
```bash
bash mkimg.sh
```

Dependencies:
- [GNU mtools](https://www.gnu.org/software/mtools/)
  - [Pre-built binaries for Windows and Linux](https://github.com/LensPlaysGames/mtools/releases) 
  - On Debian distros: `sudo apt install mtools`
  - [Source Code](http://ftp.gnu.org/gnu/mtools/)

---

### `mkiso.sh` <a name="bootmediagen-mkiso-sh"></a>
Generate a bootable disk image with an ISO-9660 filesystem.
  The contents of `LensorOS.img` are loaded onto it in the
  "El-Torito" configuration, making the `.iso` bootable.

Invocation:
```bash
bash mkiso.sh
```
Dependencies:
- [GNU xorriso](https://www.gnu.org/software/xorriso/)
  - On Debian distributions: `sudo apt install xorriso`
  - Pre-built Windows executables can be found
    [in this repository](https://github.com/PeyTy/xorriso-exe-for-windows)
  - [Source Code](https://www.gnu.org/software/xorriso/xorriso-1.5.4.pl02.tar.gz)

---

### `mkimg.bat` <a name="bootmediagen-mkimg-bat"></a>
Generate a bootable disk image that is UEFI compatible (FAT32 formatted).

Invocation:
```cmd
mkimg.bat
```

Dependencies:
- [GNU mtools](https://www.gnu.org/software/mtools/)
  - [Pre-built binaries for Windows](https://github.com/LensPlaysGames/mtools/releases)
  - [Source Code](http://ftp.gnu.org/gnu/mtools/)

---

### `mkiso.bat` <a name="bootmediagen-mkiso-bat"></a>
Generate a bootable disk image with an ISO-9660 filesystem.
  The contents of `LensorOS.img` are loaded onto it in the
  "El-Torito" configuration, making the `.iso` bootable.

Invocation:
```cmd
mkiso.bat
```

Dependencies:
- [GNU xorriso](https://www.gnu.org/software/xorriso/)
  - Windows executables can be found
    [in this repository](https://github.com/PeyTy/xorriso-exe-for-windows)
  - [Source Code](https://www.gnu.org/software/xorriso/xorriso-1.5.4.pl02.tar.gz)

---

### `mkimg.ps1` <a name="bootmediagen-mkimg-ps1"></a>
Generate a bootable disk image that is UEFI compatible (FAT32 formatted).

Invocation:
```pwsh
.\mkimg.ps1
```

Dependencies:
- [GNU mtools](https://www.gnu.org/software/mtools/)
  - [Pre-built binaries for Windows and Linux](https://github.com/LensPlaysGames/mtools/releases)
  - On Debian distros: `sudo apt install mtools`
  - [Source Code](http://ftp.gnu.org/gnu/mtools/)

---

### `mkiso.ps1` <a name="bootmediagen-mkiso-ps1"></a>
Generate a bootable disk image with an ISO-9660 filesystem.
  The contents of `LensorOS.img` are loaded onto it in the
  "El-Torito" configuration, making the `.iso` bootable.

Invocation:
```pswh
.\mkiso.ps1
```

Dependencies:
- [GNU xorriso](https://www.gnu.org/software/xorriso/)
  - On Debian distributions: `sudo apt install xorriso`
  - Pre-built Windows executables can be found
    [in this repository](https://github.com/PeyTy/xorriso-exe-for-windows)
  - [Source Code](https://www.gnu.org/software/xorriso/xorriso-1.5.4.pl02.tar.gz)

---

### `run.sh` <a name="booting-run-sh"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the raw FAT32 image, `LensorOS.img`.

There is also a debug version of the script that
  instructs QEMU to wait for a connection from the
  GNU debugger (`gdb) before starting CPU execution.

Invocation:
```bash
bash run.sh
```
```bash
bash rundbg.sh
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#linux)

---

### `runhda.sh` <a name="booting-runhda-sh"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the GPT formatted disk image, `LensorOS.bin`.

There is also a debug version of the script that
  instructs QEMU to wait for a connection from the
  GNU debugger (`gdb) before starting CPU execution.

Invocation:
```bash
bash runhda.sh
```
```bash
bash runhdadbg.sh
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#linux)

---

### `runiso.sh` <a name="booting-runiso-sh"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the ISO-9660 CD-ROM image, `LensorOS.iso`.

Invocation:
```bash
bash runiso.sh
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#linux)

---

### `run.bat` <a name="booting-run-bat"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the raw FAT32 image, `LensorOS.img`.

There is also a debug version of the script that
  instructs QEMU to wait for a connection from the
  GNU debugger (`gdb) before starting CPU execution.

Invocation:
```pwsh
.\run.bat
```
```pwsh
.\rundbg.bat
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

### `runhda.bat` <a name="booting-runhda-bat"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the GPT formatted disk image, `LensorOS.bin`.

There is also a debug version of the script that
  instructs QEMU to wait for a connection from the
  GNU debugger (`gdb) before starting CPU execution.

Invocation:
```cmd
runhda.bat
```
```cmd
runhdadbg.bat
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

### `runiso.bat` <a name="booting-runiso-bat"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the ISO-9660 CD-ROM image, `LensorOS.iso`.

Invocation:
```cmd
runiso.bat
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

### `run.ps1` <a name="booting-run-ps1"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the raw FAT32 image, `LensorOS.img`.

Invocation:
```pwsh
.\run.ps1
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

### `runhda.ps1` <a name="booting-runhda-ps1"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the GPT formatted disk image, `LensorOS.bin`.

Invocation:
```pwsh
.\runhda.ps1
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

### `runiso.ps1` <a name="booting-runiso-ps1"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the ISO-9660 CD-ROM image, `LensorOS.iso`.

Invocation:
```pwsh
.\runiso.ps1
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

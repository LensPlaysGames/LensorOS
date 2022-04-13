# LensorOS Scripts

---

### Table of Contents:
- Boot Media Generation
  - [install_mkgpt.sh](#bootmediagen-installmkgpt-sh)
  - [mkgpt.sh](#bootmediagen-mkgpt-sh)
  - [mkimg.sh](#bootmediagen-mkimg-sh)
  - [mkiso.sh](#bootmediagen-mkiso-sh)
- Booting into LensorOS
  - Linux
    - [run.sh](#booting-run-sh)
    - [rundbg.sh](#booting-rundbg-sh)
	- [runhda.sh](#booting-runhda-sh)
	- [runhdadbg.sh](#booting-runhdadbg-sh)
  - Windows
    - [run.bat](#booting-run-bat)
    - [rundbg.bat](#booting-rundbg-bat)
	- [runhda.bat](#booting-runhda-bat)
	- [runhdadbg.bat](#booting-runhdadbg-bat)
- Visualization
  - [cinclude2dot.sh](#kernel-cinclude2dot-sh)

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

### `mkimg.sh` <a name="bootmediagen-mkimg-sh"></a>
Generate a bootable disk image that is UEFI compatible (FAT32 formatted).

Invocation:
```bash
bash mkimg.sh
```

Dependencies:
- [GNU mtools](https://www.gnu.org/software/mtools/)
  - [See Lens' fork of mtools on GitHub](https://github.com/LensPlaysGames/mtools/releases) 
    with pre-built binaries for x86_64 Windows and Linux.
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

### `run.sh` <a name="booting-run-sh"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the raw FAT32 image, `LensorOS.img`.

Invocation:
```bash
bash run.sh
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#linux)

---

### `rundbg.sh` <a name="booting-rundbg-sh"></a>
Do the same as [run.sh](#booting-run-sh) but instruct QEMU to wait
  for a connection from the GNU debugger (`gdb`) before starting CPU execution.

Invocation:
```bash
bash rundbg.sh
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#linux)

---

### `runhda.sh` <a name="booting-runhda-sh"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the GPT formatted disk image, `LensorOS.bin`.

Invocation:
```bash
bash runhda.sh
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#linux)

---

### `runhdadbg.sh` <a name="booting-runhdadbg-sh"></a>
Do the same as [runhda.sh](#booting-runhda-sh) but instruct QEMU to wait
  for a connection from the GNU debugger (`gdb`) before starting CPU execution.

Invocation:
```bash
bash runhdadbg.sh
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#linux)

---

### `run.bat` <a name="booting-run-bat"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the raw FAT32 image, `LensorOS.img`.

Invocation:
```pwsh
.\run.bat
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

### `rundbg.bat` <a name="booting-rundbg-bat"></a>
Do the same as [run.bat](#booting-run-bat) but instruct QEMU to wait
  for a connection from the GNU debugger (`gdb`) before starting CPU execution.

Invocation:
```pwsh
.\rundbg.bat
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

### `runhda.bat` <a name="booting-runhda-bat"></a>
Launch QEMU with the proper flags to boot into
  LensorOS from the GPT formatted disk image, `LensorOS.bin`.

Invocation:
```pwsh
.\runhda.bat
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

### `runhdadbg.bat` <a name="booting-runhdadbg-bat"></a>
Do the same as [runhda.bat](#booting-runhda-bat) but instruct QEMU to wait
  for a connection from the GNU debugger (`gdb) before starting CPU execution.

Invocation:
```pwsh
.\runhdadbg.bat
```

Dependencies:
- [QEMU](https://www.qemu.org/download/#windows)

---

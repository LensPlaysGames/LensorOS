# LensorOS Scripts
A collection of shell scripts that automate processes surrounding LensorOS.

---

### Table of Contents:
- Kernel
  - [cinclude2dot.sh](#kernel-cinclude2dot-sh)
- Boot Media Generation
  - [install_mkgpt.sh](#bootmediagen-installmkgpt-sh)
  - [mkgpt.sh](#bootmediagen-mkgpt-sh)

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
Make a bootable disk image with a valid GUID Partition Table.

[mkimg.sh](#bootmediagen-mkimg-sh) is run before generating the
  GPT image to ensure the boot media is as up to date as possible.

Invocation:
```
bash mkgpt.sh
```

Dependencies:
- [mkgpt](#bootmediagen-install-mkgpt-sh)

---

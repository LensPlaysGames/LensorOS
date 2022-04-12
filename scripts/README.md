# LensorOS Scripts
A collection of shell scripts that automate processes surrounding LensorOS.

---

### Table of Contents:
- Kernel
  - [cinclude2dot.sh](#kernel-cinclude2dot)

---

### `cinclude2dot.sh` <a name="kernel-cinclude2dot"></a>
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

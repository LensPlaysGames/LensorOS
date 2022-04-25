set(
  ARCH "x86_64"
  CACHE STRING
  "The CPU architecture that LensorOS will be run on"
)
set_property(
  CACHE ARCH
  PROPERTY STRINGS
  "x86_64"
)
set(
  MACHINE "VBOX"
  CACHE STRING
  "The machine type that LensorOS will run on"
)
set_property(
  CACHE MACHINE
  PROPERTY STRINGS
  "PC"
  "QEMU"
  "VBOX"
  "VMWARE"
)
option(
  HIDE_UART_COLOR_CODES
  "Do not print ANSI terminal color codes to serial output.
Particularly useful if the terminal you are using does not support it."
)

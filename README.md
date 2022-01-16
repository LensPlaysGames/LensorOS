# LensorOS
An OS I've created with the help of [Poncho](https://github.com/Absurdponcho)

### REQUIRED

On Windows, `WSL` is required for this project.

If on Windows, enter a WSL terminal. \
To initialize the directories needed, cd to the `kernel` folder and run the following: \
`make setup`

Once setup is ran, copy the `.psf` font file from the repo directory into `kernel/bin`.

After this, `cd` to the `gnu-efi` folder and run the following: \
`make bootloader`

Next, `cd` to `../kernel` and run: \
`make kernel` \
followed by: \
`make buildimg`

This will generate a `.iso` image file that can be used as a boot disk in a virtual machine, like [QEMU](https://www.qemu.org/).

If on Windows, a `run.bat` file is included. Simply double click this to run QEMU, booting directly from the LensorOS bootloader. The batch file requires the directory the QEMU executable resides in be added to the system's PATH variable [see this stackoverflow thread for help](https://stackoverflow.com/questions/9546324/adding-a-directory-to-the-path-environment-variable-in-windows).

If on Linux, run `make run` and QEMU should open.
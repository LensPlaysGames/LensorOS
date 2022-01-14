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

Next, `cd` back to the `kernel` folder and run: \
`make kernel` \
followed by: \
`make buildimg`

This will generate a `.iso` image file that can be used as a boot disk in a virtual machine, like [QEMU]().
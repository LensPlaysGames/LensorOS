# LensorOS
An OS I've created with the help of [Poncho](https://github.com/Absurdponcho)

### Running LensorOS on hardware
###### DISCLAIMER: LensorOS IS IN NO WAY GUARANTEED TO BE 'SAFE'; RUN AT YOUR OWN RISK! (see LICENSE)

(coming soon)

### Running LensorOS on an emulator
On Windows, `WSL` is required.

Open a Linux terminal, then `cd` to the root directory of the repository.

To initialize the directories needed, `cd` to the `kernel` folder and run the following: \
`make setup`

Once setup is complete, copy the `.psf` font file from the repo directory into `kernel/bin`. \
I should definitely automate this, but I'm not familiar with Makefile's enough to try that yet. \
If you have the make know-how, I would gladly accept a pull request with this feature built-in.

After this, `cd` to the `gnu-efi` folder and run the following: \
`make bootloader`

Next, run: \
`cd ../kernel` \
`make kernel`

This will generate an `.efi` file from the kernel source code. 

Finally, run: \
`make buildimg`

This will generate a `.iso` image file that can be used as a boot disk, like [QEMU](https://www.qemu.org/).

If on Windows, a `run.bat` file is included. Simply double click this to run QEMU, booting directly from the LensorOS bootloader. The batch file requires the directory the QEMU executable resides in be added to the system's PATH variable [see this stackoverflow thread for help](https://stackoverflow.com/questions/9546324/adding-a-directory-to-the-path-environment-variable-in-windows).

If on Linux, run `make run` and QEMU should open.
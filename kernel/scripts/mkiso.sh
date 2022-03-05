ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BuildDirectory="$ScriptDirectory/../bin"
$ScriptDirectory/mkimg.sh
mkdir -p $BuildDirectory/iso
cp $BuildDirectory/LensorOS.img $BuildDirectory/iso
xorriso -as mkisofs -R -f -e LensorOS.img -no-emul-boot -o $BuildDirectory/LensorOS.iso $BuildDirectory/iso
rm -r $BuildDirectory/iso

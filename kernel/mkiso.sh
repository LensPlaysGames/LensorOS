ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$ScriptDirectory/mkimg.sh
mkdir -p iso
cp $ScriptDirectory/bin/LensorOS.img iso
xorriso -as mkisofs -R -f -e LensorOS.img -no-emul-boot -o $ScriptDirectory/bin/LensorOS.iso iso
rm -r iso

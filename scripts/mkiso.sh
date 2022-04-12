ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BuildDirectory="$ScriptDirectory/../kernel/bin"

run() {
    set -x
    "$@"
    { set +x; } 2>/dev/null
}

run $ScriptDirectory/mkimg.sh
run mkdir -p $BuildDirectory/iso
run cp \
	$BuildDirectory/LensorOS.img \
	$BuildDirectory/iso
run xorriso \
	-as mkisofs \
	-R -f \
	-e LensorOS.img \
	-no-emul-boot \
	-o $BuildDirectory/LensorOS.iso \
	$BuildDirectory/iso
run rm -r $BuildDirectory/iso

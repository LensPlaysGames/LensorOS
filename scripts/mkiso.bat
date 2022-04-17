SET ScriptDirectory=%0/..
SET BuildDirectory=%ScriptDirectory%/../kernel/bin
SET ScriptDirectory=%ScriptDirectory:"=%
SET BuildDirectory=%BuildDirectory:"=%
cmd /c %ScriptDirectory%/mkimg.bat
cd %BuildDirectory%
md iso
cp LensorOS.img iso/
xorriso -as mkisofs -R -f -e LensorOS.img -no-emul-boot -o LensorOS.iso iso
@echo " -> Created El-Torito ISO-9660 boot media image"
rm -r iso

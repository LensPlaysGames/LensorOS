#!/bin/bash
# Dependencies:
#   - Curl     -- `sudo apt install curl`
#   - Perl     -- `sudo apt install perl`
#   - Dot/SFDP -- `sudo apt install graphviz`
# Command Line Usage:
#   - `-i <comma separated include paths>`
#       Search these paths for any `<>` includes.
ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
KernelDirectory=$ScriptDirectory/..
VisualsDirectory=$KernelDirectory/include_visualization
mkdir -p $VisualsDirectory

while getopts i: flag
do
    case "${flag}" in
        i) IncludePaths=${OPTARG};;
    esac
done

# Set include path to current directory if include paths were not passed.
if [ -z "$IncludePaths" ] ; then
   IncludePaths=.
fi

# Download the perl script `cinclude2dot.pl` if it is not already present.
if ! [ -f $VisualsDirectory/cinclude2dot.pl ] ; then
	echo "    Downloading perl script"
	curl https://www.flourish.org/cinclude2dot/cinclude2dot > $VisualsDirectory/cinclude2dot.pl
fi

# Run the perl script, generating the `.dot` file that will be used by GraphViz to generate an image.
echo "    Generating include dependency data"
perl $VisualsDirectory/cinclude2dot.pl --src $KernelDirectory/src --include $IncludePaths > $VisualsDirectory/LensorOS.dot

# Generate a PNG from the `.dot` file using GraphViz. If it is blurry, increase `-Gdpi` value.
echo "    Generating directed graph as 'LensorOS.dot.png' (this may take a while)"
dot -Gdpi=1800 -Tpng -o $VisualsDirectory/LensorOS.dot.png $VisualsDirectory/LensorOS.dot

echo "    Generating undirected graph as 'LensorOS.sfdp.png' (this may take a while)"
sfdp -Gdpi=1800 -Tpng -o $VisualsDirectory/LensorOS.sfdp.png $VisualsDirectory/LensorOS.dot

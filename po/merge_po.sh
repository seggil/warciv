#!/bin/bash

usability() {
    echo "USAGE: ./merge_po.sh <Updated_freeciv-2.2_sourcedir>"
    echo "Get Update_freeciv-2.2_source by"
    echo "svn co http://svn.gna.org/svn/freeciv/branches/S2_2/ <Update_freeciv-2.0_source_dir>"
    exit
}

if [ "$1" == '' ]; then
    usability
else
    WORKDIR=$1
fi

for fichier in `ls *.po`; do
    echo $fichier
    msgmerge --compendium=$WORKDIR/po/$fichier $fichier freeciv.pot -o ${fichier}x
    mv ${fichier}x $fichier
done

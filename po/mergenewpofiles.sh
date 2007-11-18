#!/bin/bash

for fichier in `ls *.po`; do
    echo $fichier
    msgmerge --compendium=$fichier ../../freeciv-2.0/po/$fichier freeciv.pot -o ${fichier}x
    mv ${fichier}x $fichier
done
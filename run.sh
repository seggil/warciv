#!/bin/bash

#default dir where we merge warclient and freeciv
if [ "$1" == '' ]; then
    pwd=`pwd`
    DIRCOMPILE=$pwd/merged/
else
    DIRCOMPILE=$1
fi

#check if merged directory is avaliable
if [ -a $DIRCOMPILE ]; then
    echo Directory $DIRCOMPILE exist already
    echo "First delete it ( rm -fr $DIRCOMPILE ) or run" 
    echo "./run <your_dir>"
    exit 1
fi

echo Warclient sources and Freeciv sources will be merged into $DIRCOMPILE dir

#perform merging
mkdir $DIRCOMPILE

cp -a freeciv-2.0.9/* $DIRCOMPILE/.
rm -fr $DIRCOMPILE/debian

cp -fa warclient/* $DIRCOMPILE/.

#remove .svn/ directory for having clean sources 
rm -fr `find $DIRCOMPILE -name .svn`

#set executable 
chmod u+x $DIRCOMPILE/autogen.sh
chmod u+x $DIRCOMPILE/debian/rules

#display what it must be done
echo now run : 
echo cd $DIRCOMPILE
echo "./autogen.sh --enable-client=gtk2-0 (for example see ./configure --help)"
echo make
echo make install

exit 0

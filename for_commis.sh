#!/bin/bash
usability() {
    echo "USAGE: ./for_commis.sh <your_work_dir> <your_tmp_dir> <output_patch_file>"
    echo "default <your_tmp_dir>=warclient.new"
    echo "default <output_patch_file>=patch.diff"
    exit
}

pwd=`pwd`

#default dir where we work
if [ "$1" == '' ]; then
    usability
else
    if [ -d $1 ]; then
	WORKDIR=$1
    else
	echo $1 do not exist!! exiting
	exit
    fi
fi

if [ "$2" == '' ]; then
    TMPDIR=warclient.new
else
    TMPDIR=$2
fi

#check if merged directory is avaliable
if [ -a $TMPDIR ]; then
    echo Directory $TMPDIR exist already
    echo "First delete it ( rm -fr $TMPDIR ) or"
    echo "Change name of <output_patch_file>"
    exit 1
fi

if [ "$3" == '' ]; then
    pwd=`pwd`
    patch=$pwd/patch.diff
else
    patch=$3
fi

#check if merged directory is avaliable
if [ -a $patch ]; then
    echo File $patch exist already
    echo "First delete it ( rm -fr $patch ) or"
    echo "Change name of <output_patch_file>"
    exit 1
fi

DIRWAR=warclient
DIRFREE=freeciv-2.0

cp -a $DIRWAR $TMPDIR
#verify if PATH absolute or relative is given
cd $TMPDIR/..
if [ "$pwd" != `pwd` ]; then
    echo "<your_tmp_dir> must not contain and / ; it must be create in this directory"
    cd $pwd
    rm -fr $TMPDIR
    exit
fi
cd $pwd

for fichier in `find $DIRWAR ! -wholename '*/.svn*'  `; do

echo $fichier | grep -v '/$' | sed 's|[^/]*||' | while read -r name
  do
if [ ! -d $fichier ]; then
#  echo $fichier $WORKDIR/$name
    TEST=`diff $fichier $WORKDIR/$name`
#echo $TEST
    if [ "$TEST" != '' ]; then
	cp $WORKDIR/$name $TMPDIR/$name
	echo "diff -urNad $DIRWAR/$name $TMPDIR/$name"  >>  $patch
	diff -urNad $DIRWAR/$name $TMPDIR/$name >>  $patch
	echo "# Modified : $name"
    fi
fi

done

done

echo "*****************************************************************"
echo "*                    WARNING                                    *"
echo "*****************************************************************"
echo "if you have modified a file witch is not already in warclient dir"
echo "First copy it in warclient dir, perform svn add <file>"
echo -e "Must be fixed \n \n"
echo "Now run the following commands"
echo "cd $DIRWAR"
echo "patch -p1 < $patch"
echo -e "\n \nNow do your commis"
echo "svn ci -m \"Your job\""

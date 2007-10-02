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
    MY_TMPDIR=warclient.new
else
    MY_TMPDIR=$2
fi

#check if merged directory is avaliable
if [ -a $MY_TMPDIR ]; then
    echo Directory $MY_TMPDIR exist already
    echo "First delete it ( rm -fr $MY_TMPDIR ) or"
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

cp -a $DIRWAR $MY_TMPDIR
#verify if PATH absolute or relative is given
cd $MY_TMPDIR/..
if [ "$pwd" != `pwd` ]; then
    echo "<your_tmp_dir> must not contain and / ; it must be create in this directory"
    cd $pwd
    rm -fr $MY_TMPDIR
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
		cp $WORKDIR/$name $MY_TMPDIR/$name
		echo "diff -urNad $DIRWAR/$name $MY_TMPDIR/$name"  >>  $patch
		diff -urNad $DIRWAR/$name $MY_TMPDIR/$name >>  $patch
		echo "# Modified : $name"
	    fi
	fi
	
    done

done
    
DATE=`date +%s`
if [ ! -f $MY_TMPDIR/tmp$DATE ]; then
    touch  $MY_TMPDIR/tmp$DATE
else
    echo "Hmm, $MY_TMPDIR/tmp$DATE exist! How is it possible?"
    echo "Retry and if don't work, we found a bug"
    exit 1
fi

for fichier in `find $DIRFREE ! -wholename '*/.svn*'  `; do
    
    echo $fichier | grep -v '/$' | sed 's|[^/]*||' | while read -r name
    do
	if [ ! -d $fichier ] && [ -e $WORKDIR/$name ]; then
#  echo $fichier $WORKDIR/$name
	    TEST=`diff $fichier $WORKDIR/$name`
#echo $TEST
	    if [ "$TEST" != '' ] && [ ! -e $DIRWAR/$name ] ; then
		echo "# Modified and Added to warclient: $name"
		echo "# Modified and Added to warclient: $name" >> $MY_TMPDIR/tmp$DATE
		echo -e "cp -a $WORKDIR/$name $DIRWAR/$name" >> $MY_TMPDIR/tmp$DATE
		echo -e "svn add $DIRWAR/$name" >> $MY_TMPDIR/tmp$DATE
	    fi
	fi
    done
done

echo "*******************************************************************"
echo "*                      WARNING                                    *"
echo "*******************************************************************"
echo -e "If you have added a new file, with a name non existant"
echo -e "into warclient or freeciv dir copy it in warclient dir,"
echo -e "and perform : svn add <file>"
echo "*******************************************************************"
echo -e "\n All modified files will be introduce in tree"
echo -e "       by running the following commands : \n"
echo -e "cd $DIRWAR"
echo -e "patch -p1 < $patch"
if [ "`more $MY_TMPDIR/tmp$DATE`" != '' ]; then
echo -e "cd .."
echo -e "#You must add theses files into svn repository ; Run"
fi
cat $MY_TMPDIR/tmp$DATE
echo -e "\n\n#Now do your commis"
echo -e "svn ci -m \"Your job\""

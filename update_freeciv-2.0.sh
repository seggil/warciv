#!/bin/bash
usability() {
    echo "USAGE: ./update_freeciv-2.0.sh <your_tmp_dir> <Updated_freeciv-2.0_sourcedir>"
    echo "Get Update_freeciv-2.0_source by"
    echo "svn co http://svn.gna.org/svn/freeciv/branches/S2_0/ <Update_freeciv-2.0_source_dir>"
    exit
}

pwd=`pwd`

#default dir where we work
if [ "$1" == '' ]; then
    usability
else
    WORKDIR=$1
fi

#check if merged directory is avaliable
if [ -a $WORKDIR ]; then
    echo Directory $WORKDIR exist already
    echo "First delete it ( rm -fr $WORKDIR ) or run"
    exit 1
fi

if [ "$2" == '' ]; then
    echo "No new sources of freeciv given"
    usability
fi

if [ -d $pwd/$2 ]; then
#relative PATH given
    NEWFREECIV=$pwd/$2
elif [ -d $2 ]; then
#Full Path
    NEWFREECIV=$2
else
    echo "Directory $2 don't exist"
    usability
    exit
fi

mkdir $WORKDIR || exit
cd $WORKDIR

DIR=freeciv-2.0

cp -a $pwd/freeciv-2.0 $DIR.orig
cp -a $NEWFREECIV $DIR
rm -fr `find . -name .svn`

diff -urNad $DIR.orig $DIR > freeciv.diff


cp -a $DIR.orig/client/gui-gtk-2.0 $DIR.orig/client/gui-warclient-gtk-2.0
cp -a $DIR/client/gui-gtk-2.0 $DIR/client/gui-warclient-gtk-2.0

DIRWAR=warclient
cp -a $pwd/$DIRWAR .
rm -fr `find . -name .svn`

DIRWAR_LIKE=warclient_like
DIRWAR_UPDATED=warclient_updated

echo "#!/bin/bash" >> output.sh
for fichier in `find $DIRWAR ! -wholename '*/.svn*'  `; do

echo $fichier | grep -v '/$' | sed 's|[^/]*||' | while read -r name
do
TEST=$DIR.orig/$name
#echo $TEST
if [ -f $TEST ]; then
    echo cp $TEST $DIRWAR_LIKE/$name >>   output.sh
elif [ -d $TEST ]; then
    echo mkdir $DIRWAR_LIKE/$name >>   output.sh
fi
TEST=$DIR/$name
#echo $TEST
if [ -f $TEST ]; then
    echo cp $TEST $DIRWAR_UPDATED/$name >>   output.sh
elif [ -d $TEST ]; then
    echo mkdir $DIRWAR_UPDATED/$name >>   output.sh
fi
done

done

echo "rm -fr */po/*.po"   >>   output.sh
echo "rm -fr */debian"  >>   output.sh
echo "diff -urNadEBbw $DIRWAR_LIKE $DIRWAR > warclient_old.diff"  >>   output.sh

echo cd $DIRWAR_UPDATED  >>   output.sh

echo "patch -p1 -f < ../warclient_old.diff"  >>   output.sh

echo "cd -"  >>   output.sh

echo "diff -urNad $DIRWAR $DIRWAR_UPDATED > warclient.diff"  >>   output.sh

echo "echo verifie that patch warclient_old.diff is apply correctly"  >>   output.sh
echo "echo if yes apply warclient.diff freeciv.diff " >>   output.sh
echo "echo if not, patch anyway and fix all .rej failed patch"  >>   output.sh
echo "echo cd $pwd/warclient" >>   output.sh
echo "echo \"patch -p1 < ../$1/warclient.diff\"" >>   output.sh
echo "echo cd $pwd/$DIR" >>   output.sh
echo "echo \"patch -p1 < ../$1/freeciv.diff\"" >>   output.sh
echo "echo cd .." >>   output.sh
echo "echo \"#Warning your svn dir $pwd/warclient can contain somme .orig file\""  >>   output.sh
echo "echo \"#You can delete them by: rm -ivr \\\` find . -wholename \\\"*orig\\\"\\\` \""  >>   output.sh
echo "echo \"#Warning your svn dir $pwd/warclient can contain somme .rej file\""  >>   output.sh
echo "echo \"#You can delete them by: rm -ivr \\\` find . -wholename \\\"*rej\\\"\\\` \""  >>   output.sh
echo "echo \"#After you run for be sure that new files will be take in account\"" >>   output.sh

chmod u+x output.sh

echo Go into $1 dir and run ./output.sh

cd $pwd

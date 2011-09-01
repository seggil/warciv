#!/bin/sh

# Freeciv - Copyright (C) 2007 - The Freeciv Project

# This script generates wc_svnrev_gen.h from wc_svnrev_gen.h.in.
# See wc_svnrev_gen.h.in for details.

# Parameters - $1 - top srcdir
#              $2 - top builddir
#

# Absolete paths
SRCROOT=$(cd "$1" ; pwd)
INPUTDIR=$(cd "$1/bootstrap" ; pwd)
OUTPUTDIR=$(cd "$2/common" ; pwd)

REVSTATE="OFF"
REV="dist"

if test -e "$1/NO_GEN_SVNREV" ; then
  echo "NO_GEN_SVNREV"
  exit
fi

(echo "GENERATE_SVN_REV"
 cd "$INPUTDIR"
 # Check that all commands required by this script are available
 # If not, we will not claim to know which svn revision this is
 # (REVSTATE will be OFF)
 if which svn && which tail && which wc ; then
   REVTMP="r$(LANG=C svn info 2>/dev/null | grep "^Revision: " | sed 's/^Revision: //')"
   if test "$REVTMP" != "r" ; then
     # This is svn checkout. Check for local modifications
     if test $(cd "$SRCROOT" ; svn diff | wc -l) -eq 0 ; then
       REVSTATE=ON
       REV="$REVTMP"
     else
       REVSTATE=MOD
       REV="modified $REVTMP"
     fi
   fi
 fi

 if test "$REVSTATE" = "OFF" && which git-svn && which wc ; then
   REVTMP="$(git-svn log -n 1 --oneline 2>/dev/null | awk -F ' ' '{print $1}')"
   if test -n "$REVTMP" ; then
     if test "$(git-rev-list -n 1 HEAD)" = "$(git-svn find-rev $REVTMP)" -a $(git-diff | wc -l) -eq 0 ; then
       REVSTATE=ON
       REV="$REVTMP"
     else
       REVSTATE=MOD
       REV="modified $REVTMP"
     fi
   fi
 fi

 sed -e "s,<SVNREV>,$REV," -e "s,<SVNREVSTATE>,$REVSTATE," wc_svnrev_gen.h.in > "$OUTPUTDIR/wc_svnrev_gen.h.tmp"
 if ! test -f "$OUTPUTDIR/wc_svnrev_gen.h" || ! cmp "$OUTPUTDIR/wc_svnrev_gen.h" "$OUTPUTDIR/wc_svnrev_gen.h.tmp" ; then
   mv "$OUTPUTDIR/wc_svnrev_gen.h.tmp" "$OUTPUTDIR/wc_svnrev_gen.h"
 fi
 rm -f "$OUTPUTDIR/wc_svnrev_gen.h.tmp"
) > /dev/null


#!/bin/sh

cat <<EOF
!include "MUI.nsh"

;General

!define APPNAME "Pepclient"
!define APPID "Pepclient-2.0.9+0.9.5Devel-sdl"
Name "Pepclient 2.0.9+0.9.5Devel (SDL client)"
OutFile "Output/Pepclient 2.0.9+0.9.5Devel-setup.exe"
InstallDir "\$PROGRAMFILES\\\${APPID}"

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\\\${APPID} "Install_Dir"

;Variables

Var STARTMENU_FOLDER

; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "$1\COPYING"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU "Application" \$STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "\$INSTDIR\civclient.exe"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;Languages

!insertmacro MUI_LANGUAGE "English"

EOF

### required files ###

cat <<EOF
; The stuff to install
Section "\${APPNAME} (required)"

  SectionIn RO

  SetOutPath \$INSTDIR
EOF

  # find files and directories to exclude from default installation

  echo -n "  File /nonfatal /r "

  # languages
  echo -n "/x locale "

  # soundsets
  find $1/data -name *.soundspec -mindepth 1 -maxdepth 1 -printf %f\\n |
  sed 's|.soundspec||' |
  while read -r name
  do
  echo -n "/x $name.soundspec /x $name "
  done

  echo "$1\\*.*"

cat <<EOF
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\\\${APPID} "Install_Dir" "\$INSTDIR"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "\$SMPROGRAMS\\\$STARTMENU_FOLDER"
  CreateShortCut "\$SMPROGRAMS\\\$STARTMENU_FOLDER\Pepclient Server.lnk" "\$INSTDIR\civserver.exe"
  CreateShortCut "\$SMPROGRAMS\\\$STARTMENU_FOLDER\Pepclient.lnk" "\$INSTDIR\civclient.exe"
  CreateShortCut "\$SMPROGRAMS\\\$STARTMENU_FOLDER\Uninstall.lnk" "\$INSTDIR\uninstall.exe"
  CreateShortCut "\$SMPROGRAMS\\\$STARTMENU_FOLDER\Website Pepclient.lnk" "\$INSTDIR\Pepclient.url"
  CreateShortCut "\$SMPROGRAMS\\\$STARTMENU_FOLDER\Website Freeciv.lnk" "\$INSTDIR\Freeciv.url"
  !insertmacro MUI_STARTMENU_WRITE_END

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}" "DisplayName" "\$(^Name)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}" "UninstallString" '"\$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
  SetOutPath \$INSTDIR
SectionEnd

EOF

### soundsets ###

cat <<EOF
SectionGroup "soundsets"

EOF

find $1/data -name *.soundspec -mindepth 1 -maxdepth 1 -printf %f\\n |
sort |
sed 's|.soundspec||' |
while read -r name
do
if test -d $1/data/$name; then
echo "  Section \"$name\""
echo "  SetOutPath \$INSTDIR\\data"
echo "  File /r $1\data\\$name.soundspec"
echo "  SetOutPath \$INSTDIR\\data\\$name"
echo "  File /r $1\\data\\$name\*.*"
echo "  SetOutPath \$INSTDIR"
echo "  SectionEnd"
echo
fi
done

cat <<EOF
SectionGroupEnd

EOF

### additional languages ###

cat <<EOF
SectionGroup "additional languages"

EOF

find $1/share/locale -type d -mindepth 1 -maxdepth 1 -printf %f\\n |
sort |
while read -r name
do
echo "  Section /o \"$name\""
echo "  SetOutPath \$INSTDIR\\share\\locale\\$name"
echo "  File /r $1\\share\\locale\\$name\*.*"
if test -d $1/lib/locale/$name; then
echo "  SetOutPath \$INSTDIR\\lib\\locale\\$name"
echo "  File /r $1\\lib\\locale\\$name\*.*"
fi
echo "  SetOutPath \$INSTDIR"
echo "  SectionEnd"
echo
done

cat <<EOF
SectionGroupEnd

EOF

### uninstall section ###

cat <<EOF
; special uninstall section.
Section "Uninstall"
  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}"
  DeleteRegKey HKLM SOFTWARE\\\${APPID}

  ; remove files
EOF

find $1 -type f |
grep pepclient | grep -v '/$' |
sed 's|[^/]*||' |
tr '/' '\\' | while read -r name
do
echo "  Delete \"\$INSTDIR$name\""
done

find $1 -type d -depth |
grep pepclient | grep -v '/$' |
sed 's|[^/]*||' |
tr '/' '\\' | while read -r name
do
echo "  RMDir \"\$INSTDIR$name\""
done

cat <<EOF

  ; MUST REMOVE UNINSTALLER, too
  Delete \$INSTDIR\uninstall.exe

  ; remove shortcuts, if any.
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" \$STARTMENU_FOLDER
  Delete "\$SMPROGRAMS\\\$STARTMENU_FOLDER\*.*"

  ; remove directories used.
  RMDir "\$SMPROGRAMS\\\$STARTMENU_FOLDER"
  RMDir "\$INSTDIR"
SectionEnd
EOF


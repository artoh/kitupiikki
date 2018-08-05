#!/bin/bash

#
#   Linux release (AppImage)
#   Vaatii linuxdeployqt:n
#

# Käännös suoritetaan Ubuntu 14.04 virtuaalikoneessa

BUILDIR=/home/arto/kitupiikki/build
QTDIR=/opt/qt510
DISTDIR=/media/sf_virtuaali

cd $BUILDIR

$QTDIR/bin/qmake ../kitupiikki/kitupiikki.pro -spec linux-g++ "CONFIG+=release" && /usr/bin/make qmake_all

make -j 6
make clean

cp ../kitupiikki.desktop .
cp ../kitupiikki.png .

/opt/linuxdeployqt-continuous-x86_64.AppImage kitupiikki.desktop -appimage -bundle-non-qt-libs -qmake=/$QTDIR/bin/qmake -verbose=2 -no-translations -no-copy-copyright-files

cp Kitupiikki-x86_64.AppImage ../$DIST_DIR/Kitupiikki-x.x-x86_64.AppImage

cd ~

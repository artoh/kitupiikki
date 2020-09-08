#!/bin/bash

#
#   Linux release (AppImage)
#   Vaatii linuxdeployqt:n
#

# Käännös suoritetaan Ubuntu 14.04 virtuaalikoneessa

BUILDIR=build
QTDIR=/opt/qt512
DISTDIR=/media/sf_virtuaali

export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
export ARCH=x86:64
export VERSION=2.00-pre1

rm -R $BUILDIR
mkdir -p $BUILDIR 
cd $BUILDIR

$QTDIR/bin/qmake ../kitsas/kitsas.pro -spec linux-g++ "CONFIG+=release" && make qmake_all

make -j 6
make clean

cp ../kitsas.desktop .
cp ../kitsas.png .
ln -s kitsas AppRun

~/linuxdeployqt-6-x86_64.AppImage kitsas.desktop -appimage -bundle-non-qt-libs -qmake=$QTDIR/bin/qmake -verbose=2 -no-translations -no-copy-copyright-files

cp Kitsas* $DISTDIR

cd ..

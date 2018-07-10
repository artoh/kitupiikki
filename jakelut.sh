#!/bin/bash

#
#   Linux release (AppImage)
#   Vaatii linuxdeployqt:n
#

DIST_DIR=dist/
BUILD_DIR=build-linux
QTDIR=~/Qt/5.11.1/gcc_64

mkdir -p $DIST_DIR
mkdir -p $BUILD_DIR

cd $BUILD_DIR

$QTDIR/bin/qmake ../kitupiikki/kitupiikki.pro -spec linux-g++ "CONFIG+=release" && /usr/bin/make qmake_all

make -j 6
make clean

cp ../kitupiikki.desktop .
cp ../kitupiikki.png .

/opt/linuxdeployqt-continuous-x86_64.AppImage kitupiikki.desktop -appimage -bundle-non-qt-libs -qmake=/$QTDIR/bin/qmake -verbose=2 -no-translations -no-plugins

cp Kitupiikki-x86_64.AppImage ../$DIST_DIR/Kitupiikki-x.x-x86_64.AppImage


cd ..

#
#   Windows release 
#

BUILD_DIR=build-windows

mkdir -p $BUILD_DIR
cd $BUILD_DIR

export PATH=/opt/mxe/usr/bin:$PATH

/opt/mxe/usr/i686-w64-mingw32.static/qt5/bin/qmake ../kitupiikki/kitupiikki.pro  "CONFIG += release"
make -j 6
cp release/kitupiikki.exe ../$DIST_DIR


#!/bin/bash

#
#   Windows release 
#

BUILD_DIR=build-windows
DIST_DIR=dist

mkdir -p $BUILD_DIR
mkdir -p $DIST_DIR
cd $BUILD_DIR

export PATH=/home/arto/mxe/usr/bin:$PATH

touch ../kitupiikki/versio.h

~/mxe/usr/i686-w64-mingw32.static/qt5/bin/qmake ../kitupiikki/kitupiikki.pro "CONFIG += release"
make -j 6
cp kitsas.exe ../$DIST_DIR

cd ../$DIST_DIR

upx -9 -f -o kitsas-X.X.exe kitsas.exe

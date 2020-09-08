#!/bin/bash

#
#   Windowsin kääntäminen MXE-ristiinkääntöympäristössä
#

BUILD_DIR=build-windows
DIST_DIR=dist
MXE_DIR=~/mxe

mkdir -p $BUILD_DIR
mkdir -p $DIST_DIR
cd $BUILD_DIR

export PATH=$MXE_DIR/usr/bin:$PATH

touch ../kitsas/versio.h

$MXE_DIR/usr/i686-w64-mingw32.static/qt5/bin/qmake ../kitsas/kitsas.pro "CONFIG += release"
make -j 6
cp release/kitsas.exe ../$DIST_DIR

cd ../$DIST_DIR

upx -9 -f -o kitsas-X.X.exe kitsas.exe

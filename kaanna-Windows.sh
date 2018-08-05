#!/bin/bash

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


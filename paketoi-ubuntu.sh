#!/bin/bash

BUILDIR=build-linux-qt6
QMAKE=~/Qt6/6.4.2/gcc_64/bin/qmake
DISTDIR=../dist
VERSION=5.6

export QMAKE=$QMAKE

clear
# rm -R $BUILDIR
# mkdir -p $BUILDIR 
cd $BUILDIR

# $QMAKE ../kitsas/kitsas.pro -spec linux-g++ "CONFIG+=release" && make qmake_all

# make -j 6

# wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

# wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

chmod +x linuxdeploy*.AppImage

./linuxdeploy-x86_64.AppImage --appdir AppDir -e kitsas -i ../kitsas.png -d ../kitsas.desktop --plugin qt -l /usr/lib/x86_64-linux-gnu/nss/libsoftokn3.so  -l /usr/lib/x86_64-linux-gnu/nss/libnssckbi.so -l /usr/lib/x86_64-linux-gnu/libssl.so.1.1 --output appimage

cp Kitsas*.AppImage $DISTDIR/Kitsas-$VERSION-x86_64.AppImage 

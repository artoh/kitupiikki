!/bin/bash

BUILDIR=build-linux-qt6
QMAKE=~/Qt/6.9.3/gcc_64/bin/qmake

DISTDIR=../dist
VERSION=5.11
LIBSSLPATH=/usr/lib/x86_64-linux-gnu/libssl.so.3
LIBCRYPTOPATH=/usr/lib/x86_64-linux-gnu/libcrypto.so.3

export QMAKE=$QMAKE


rm -R $BUILDIR
mkdir -p $BUILDIR 
cd $BUILDIR

$QMAKE ../kitsas/kitsas.pro -spec linux-g++ "CONFIG+=release" && make qmake_all

make -j 20

wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

chmod +x linuxdeploy*.AppImage

./linuxdeploy-x86_64.AppImage --appdir AppDir -e kitsas -i ../kitsas.png -d ../kitsas.desktop \
    -l $LIBSSLPATH \
    -l $LIBCRYPTOPATH \

./linuxdeploy-plugin-qt-x86_64.AppImage --appdir AppDir \
    --exclude-library libqsqlmimer.so \
    --exclude-library libqsqlodbc.so \
    --exclude-library libqsqlmysql.so \
    --exclude-library libqsqlpsql.so \
    --exclude-library libqsqloci.so \
    --exclude-library libqsqlibase.so

./linuxdeploy-x86_64.AppImage --appdir AppDir --output appimage

cp Kitsas*.AppImage $DISTDIR/Kitsas-$VERSION-x86_64.AppImage


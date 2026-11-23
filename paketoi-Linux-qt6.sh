#!/bin/bash

BUILDIR=build-linux-qt6
QMAKE=~/Qt/6.12.0/gcc_64/bin/qmake

DISTDIR=../dist
VERSION=5.12-devel-1
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

# Ilman tätä libqgtk3.so jää pois, eikä sovellus seuraa järjestelmän tummaa/vaaleaa teemaa
export DEPLOY_PLATFORM_THEMES=1

# GTK ja sen riippuvuudet on otettava käyttäjän koneelta, jotta teemat ja asetukset toimivat
GTK_POISTOT=(
    --exclude-library 'libgtk-3.so*'
    --exclude-library 'libgdk-3.so*'
    --exclude-library 'libgdk_pixbuf-2.0.so*'
    --exclude-library 'libatk-1.0.so*'
    --exclude-library 'libatk-bridge-2.0.so*'
    --exclude-library 'libcairo.so*'
    --exclude-library 'libcairo-gobject.so*'
    --exclude-library 'libpango-1.0.so*'
    --exclude-library 'libpangocairo-1.0.so*'
    --exclude-library 'libpangoft2-1.0.so*'
    --exclude-library 'libepoxy.so*'
    --exclude-library 'libwayland-cursor.so*'
    --exclude-library 'libwayland-egl.so*'
    --exclude-library 'libwayland-server.so*'
)

./linuxdeploy-plugin-qt-x86_64.AppImage --appdir AppDir \
    --exclude-library libqsqlmimer.so \
    --exclude-library libqsqlodbc.so \
    --exclude-library libqsqlmysql.so \
    --exclude-library libqsqlpsql.so \
    --exclude-library libqsqloci.so \
    --exclude-library libqsqlibase.so \
    "${GTK_POISTOT[@]}"

./linuxdeploy-x86_64.AppImage --appdir AppDir "${GTK_POISTOT[@]}" --output appimage

mkdir -p $DISTDIR
cp Kitsas*.AppImage $DISTDIR/Kitsas-$VERSION-x86_64.AppImage


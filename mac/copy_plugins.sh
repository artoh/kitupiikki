#!/bin/sh

#  libcopy.sh
#  kitupiikki
#
#  Created by Petri Aarnio on 28/09/2018.
#  

APPCONTENTS=$1/kitupiikki.app/Contents
APPDIR=$APPCONTENTS/MacOS

BEARER=$APPDIR/bearer
ICONENGINES=$APPDIR/iconengines
IMAGEFORMATS=$APPDIR/imageformats
PLATFORMS=$APPDIR/platforms
PRINTSUPPORT=$APPDIR/printsupport
SQLDRIVERS=$APPDIR/sqldrivers
STYLES=$APPDIR/styles

QT_VERSION=5.11.0

mkdir -p $BEARER/
mkdir -p $ICONENGINES/
mkdir -p $IMAGEFORMATS/
mkdir -p $PLATFORMS/
mkdir -p $PRINTSUPPORT/
mkdir -p $SQLDRIVERS/
mkdir -p $STYLES/

cp ~/Qt/$QT_VERSION/clang_64/plugins/bearer/libqgenericbearer.dylib $BEARER
cp ~/Qt/$QT_VERSION/clang_64/plugins/iconengines/libqsvgicon.dylib $ICONENGINES
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqgif.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqicns.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqico.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqjpeg.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqmacheif.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqmacjp2.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqsvg.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqtga.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqtiff.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqwbmp.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/imageformats/libqwebp.dylib $IMAGEFORMATS
cp ~/Qt/$QT_VERSION/clang_64/plugins/platforms/libqcocoa.dylib $PLATFORMS
cp ~/Qt/$QT_VERSION/clang_64/plugins/printsupport/libcocoaprintersupport.dylib $PRINTSUPPORT
cp ~/Qt/$QT_VERSION/clang_64/plugins/sqldrivers/libqsqlite.dylib $SQLDRIVERS
cp ~/Qt/$QT_VERSION/clang_64/plugins/styles/libqmacstyle.dylib $STYLES

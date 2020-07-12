#!/bin/sh

#  libcopy.sh
#  kitsas
#
#  Created by Petri Aarnio on 28/09/2018.
#  

APPCONTENTS=$1/kitsas.app/Contents
APPDIR=$APPCONTENTS/MacOS

BEARER=$APPDIR/bearer
ICONENGINES=$APPDIR/iconengines
IMAGEFORMATS=$APPDIR/imageformats
PLATFORMS=$APPDIR/platforms
PRINTSUPPORT=$APPDIR/printsupport
SQLDRIVERS=$APPDIR/sqldrivers
STYLES=$APPDIR/styles

QT_DIR=/usr/local/opt/qt/plugins

mkdir -p $BEARER/
mkdir -p $ICONENGINES/
mkdir -p $IMAGEFORMATS/
mkdir -p $PLATFORMS/
mkdir -p $PRINTSUPPORT/
mkdir -p $SQLDRIVERS/
mkdir -p $STYLES/

cp $QT_DIR/bearer/libqgenericbearer.dylib $BEARER
cp $QT_DIR/iconengines/libqsvgicon.dylib $ICONENGINES
cp $QT_DIR/imageformats/libqgif.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqicns.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqico.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqjpeg.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqmacheif.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqmacjp2.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqsvg.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqtga.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqtiff.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqwbmp.dylib $IMAGEFORMATS
cp $QT_DIR/imageformats/libqwebp.dylib $IMAGEFORMATS
cp $QT_DIR/platforms/libqcocoa.dylib $PLATFORMS
cp $QT_DIR/printsupport/libcocoaprintersupport.dylib $PRINTSUPPORT
cp $QT_DIR/sqldrivers/libqsqlite.dylib $SQLDRIVERS
cp $QT_DIR/styles/libqmacstyle.dylib $STYLES

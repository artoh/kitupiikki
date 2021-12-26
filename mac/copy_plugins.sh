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

QT_LOCATION=/usr/local/opt/qt5

mkdir -p $BEARER/
mkdir -p $ICONENGINES/
mkdir -p $IMAGEFORMATS/
mkdir -p $PLATFORMS/
mkdir -p $PRINTSUPPORT/
mkdir -p $SQLDRIVERS/
mkdir -p $STYLES/

cp $QT_LOCATION/plugins/bearer/libqgenericbearer.dylib $BEARER
cp $QT_LOCATION/plugins/iconengines/libqsvgicon.dylib $ICONENGINES
cp $QT_LOCATION/plugins/imageformats/libqgif.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqicns.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqico.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqjpeg.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqmacheif.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqmacjp2.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqsvg.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqtga.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqtiff.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqwbmp.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/imageformats/libqwebp.dylib $IMAGEFORMATS
cp $QT_LOCATION/plugins/platforms/libqcocoa.dylib $PLATFORMS
cp $QT_LOCATION/plugins/printsupport/libcocoaprintersupport.dylib $PRINTSUPPORT
cp $QT_LOCATION/plugins/sqldrivers/libqsqlite.dylib $SQLDRIVERS
cp $QT_LOCATION/plugins/styles/libqmacstyle.dylib $STYLES

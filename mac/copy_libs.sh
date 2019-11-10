#!/bin/sh

#  libcopy.sh
#  kitupiikki
#
#  Created by Petri Aarnio on 29/06/2018.
#  

APPNAME=kitsas
APPCONTENTS=$1/$APPNAME.app/Contents
APPDIR=$APPCONTENTS/MacOS
LIBDIR=$APPCONTENTS/Libraries

mkdir -p $LIBDIR/

#app
install_name_tool -change /usr/local/opt/poppler/lib/libpoppler-qt5.1.dylib @rpath/libpoppler-qt5.dylib $APPDIR/$APPNAME
install_name_tool -change /usr/local/opt/libzip/lib/libzip.5.dylib @rpath/libzip.dylib $APPDIR/$APPNAME

#libpoppler-qt5
cp /usr/local/opt/poppler/lib/libpoppler-qt5.dylib $LIBDIR
chmod +w $LIBDIR/libpoppler-qt5.dylib
install_name_tool -id @rpath/libpoppler-qt5.dylib $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/Cellar/poppler/0.68.0/lib/libpoppler.79.dylib @rpath/libpoppler.dylib $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @rpath/QtGui.framework/Versions/5/QtGui $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/opt/qt/lib/QtXml.framework/Versions/5/QtXml @rpath/QtXml.framework/Versions/5/QtXml $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @rpath/QtCore.framework/Versions/5/QtCore $LIBDIR/libpoppler-qt5.dylib

#libpoppler
cp /usr/local/opt/poppler/lib/libpoppler.dylib $LIBDIR
chmod +w $LIBDIR/libpoppler.dylib
install_name_tool -id @rpath/libpoppler.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/freetype/lib/libfreetype.6.dylib @rpath/libfreetype.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/fontconfig/lib/libfontconfig.1.dylib @rpath/libfontconfig.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/jpeg/lib/libjpeg.9.dylib @rpath/libjpeg.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/openjpeg/lib/libopenjp2.7.dylib @rpath/libopenjp2.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @rpath/libpng.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/libtiff/lib/libtiff.5.dylib @rpath/libtiff.dylib $LIBDIR/libpoppler.dylib

#libzip
cp /usr/local/lib/libzip.dylib $LIBDIR
chmod +w $LIBDIR/libzip.dylib
install_name_tool -id @rpath/libzip.dylib $LIBDIR/libzip.dylib

#libfreetype
cp /usr/local/opt/freetype/lib/libfreetype.dylib $LIBDIR
chmod +w $LIBDIR/libfreetype.dylib
install_name_tool -id @rpath/libfreetype.dylib $LIBDIR/libfreetype.dylib
install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @rpath/libpng.dylib $LIBDIR/libfreetype.dylib

#libfontconfig
cp /usr/local/opt/fontconfig/lib/libfontconfig.dylib $LIBDIR
chmod +w $LIBDIR/libfontconfig.dylib
install_name_tool -id @rpath/libfontconfig.dylib $LIBDIR/libfontconfig.dylib
install_name_tool -change /usr/local/opt/freetype/lib/libfreetype.6.dylib @rpath/libfreetype.dylib $LIBDIR/libfontconfig.dylib

#libjpeg
cp /usr/local/opt/jpeg/lib/libjpeg.dylib $LIBDIR
chmod +w $LIBDIR/libjpeg.dylib
install_name_tool -id @rpath/libjpeg.dylib $LIBDIR/libjpeg.dylib

#libopenjp2
cp /usr/local/opt/openjpeg/lib/libopenjp2.dylib $LIBDIR
chmod +w $LIBDIR/libopenjp2.dylib
install_name_tool -id @rpath/libopenjp2.dylib $LIBDIR/libopenjp2.dylib

#libpng
cp /usr/local/opt/libpng/lib/libpng.dylib $LIBDIR
chmod +w $LIBDIR/libpng.dylib
install_name_tool -id @rpath/libpng.dylib $LIBDIR/libpng.dylib

#libtiff
cp /usr/local/opt/libtiff/lib/libtiff.dylib $LIBDIR
chmod +w $LIBDIR/libtiff.dylib
install_name_tool -id @rpath/libtiff.dylib $LIBDIR/libtiff.dylib
install_name_tool -change /usr/local/opt/jpeg/lib/libjpeg.9.dylib @rpath/libjpeg.dylib $LIBDIR/libtiff.dylib

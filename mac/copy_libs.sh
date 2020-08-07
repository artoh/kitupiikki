#!/bin/sh

#  libcopy.sh
#  kitsas
#
#  Created by Petri Aarnio on 29/06/2018.
#  

APPNAME=Kitsas
APPCONTENTS=$1/$APPNAME.app/Contents
APPDIR=$APPCONTENTS/MacOS
LIBDIR=$APPCONTENTS/Libraries

mkdir -p $LIBDIR/

#app
install_name_tool -change /usr/local/opt/poppler/lib/libpoppler.101.dylib @rpath/libpoppler.dylib $APPDIR/$APPNAME
install_name_tool -change /usr/local/opt/poppler/lib/libpoppler-qt5.1.dylib @rpath/libpoppler-qt5.dylib $APPDIR/$APPNAME
install_name_tool -change /usr/local/opt/libzip/lib/libzip.5.dylib @rpath/libzip.dylib $APPDIR/$APPNAME

#libpoppler-qt5
cp /usr/local/opt/poppler/lib/libpoppler-qt5.dylib $LIBDIR
chmod +w $LIBDIR/libpoppler-qt5.dylib
install_name_tool -id @rpath/libpoppler-qt5.dylib $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/Cellar/poppler/0.90.1/lib/libpoppler.101.dylib @rpath/libpoppler.dylib $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @rpath/QtGui.framework/Versions/5/QtGui $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/opt/qt/lib/QtXml.framework/Versions/5/QtXml @rpath/QtXml.framework/Versions/5/QtXml $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @rpath/QtCore.framework/Versions/5/QtCore $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/opt/freetype/lib/libfreetype.6.dylib @rpath/libfreetype.dylib $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/opt/little-cms2/lib/liblcms2.2.dylib @rpath/liblcms2.dylib $LIBDIR/libpoppler-qt5.dylib

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
install_name_tool -change /usr/local/opt/little-cms2/lib/liblcms2.2.dylib @rpath/liblcms2.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nss/lib/libnss3.dylib @rpath/libnss3.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nss/lib/libnssutil3.dylib @rpath/libnssutil3.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nss/lib/libsmime3.dylib @rpath/libsmime3.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nss/lib/libssl3.dylib @rpath/libssl3.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds4.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc4.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr4.dylib $LIBDIR/libpoppler.dylib

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

#liblcms2
cp /usr/local/opt/little-cms2/lib/liblcms2.dylib $LIBDIR
chmod +w $LIBDIR/liblcms2.dylib
install_name_tool -id @rpath/liblcms2.dylib $LIBDIR/liblcms2.dylib

#libnss3
cp /usr/local/opt/nss/lib/libnss3.dylib $LIBDIR
chmod +w $LIBDIR/libnss3.dylib
install_name_tool -id @rpath/libnss3.dylib $LIBDIR/libnss3.dylib
install_name_tool -change /usr/local/Cellar/nss/3.55/lib/libnssutil3.dylib @rpath/libnssutil3.dylib $LIBDIR/libnss3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc4.dylib $LIBDIR/libnss3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds4.dylib $LIBDIR/libnss3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr4.dylib $LIBDIR/libnss3.dylib

#libnssutil3
cp /usr/local/opt/nss/lib/libnssutil3.dylib $LIBDIR
chmod +w $LIBDIR/libnssutil3.dylib
install_name_tool -id @rpath/libnssutil3.dylib $LIBDIR/libnssutil3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc4.dylib $LIBDIR/libnssutil3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds4.dylib $LIBDIR/libnssutil3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr4.dylib $LIBDIR/libnssutil3.dylib

#libsmime3
cp /usr/local/opt/nss/lib/libsmime3.dylib $LIBDIR
chmod +w $LIBDIR/libsmime3.dylib
install_name_tool -id @rpath/libsmime3.dylib $LIBDIR/libsmime3.dylib
install_name_tool -change /usr/local/Cellar/nss/3.55/lib/libnss3.dylib @rpath/libnss3.dylib $LIBDIR/libsmime3.dylib
install_name_tool -change /usr/local/Cellar/nss/3.55/lib/libnssutil3.dylib @rpath/libnssutil3.dylib $LIBDIR/libsmime3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc4.dylib $LIBDIR/libsmime3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds4.dylib $LIBDIR/libsmime3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr4.dylib $LIBDIR/libsmime3.dylib

#libssl3
cp /usr/local/opt/nss/lib/libssl3.dylib $LIBDIR
chmod +w $LIBDIR/libssl3.dylib
install_name_tool -id @rpath/libssl3.dylib $LIBDIR/libssl3.dylib
install_name_tool -change /usr/local/Cellar/nss/3.55/lib/libnss3.dylib @rpath/libnss3.dylib $LIBDIR/libssl3.dylib
install_name_tool -change /usr/local/Cellar/nss/3.55/lib/libnssutil3.dylib @rpath/libnssutil3.dylib $LIBDIR/libssl3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc4.dylib $LIBDIR/libssl3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds4.dylib $LIBDIR/libssl3.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr4.dylib $LIBDIR/libssl3.dylib

#libplds4
cp /usr/local/opt/nspr/lib/libplds4.dylib $LIBDIR
chmod +w $LIBDIR/libplds4.dylib
install_name_tool -id @rpath/libplds4.dylib $LIBDIR/libplds4.dylib
install_name_tool -change /usr/local/Cellar/nspr/4.27/lib/libnspr4.dylib @rpath/libnspr4.dylib $LIBDIR/libplds4.dylib

#libplc4
cp /usr/local/opt/nspr/lib/libplc4.dylib $LIBDIR
chmod +w $LIBDIR/libplc4.dylib
install_name_tool -id @rpath/libplc4.dylib $LIBDIR/libplc4.dylib
install_name_tool -change /usr/local/Cellar/nspr/4.27/lib/libnspr4.dylib @rpath/libnspr4.dylib $LIBDIR/libplc4.dylib

#libnspr4
cp /usr/local/opt/nspr/lib/libnspr4.dylib $LIBDIR
chmod +w $LIBDIR/libnspr4.dylib
install_name_tool -id @rpath/libnspr4.dylib $LIBDIR/libnspr4.dylib

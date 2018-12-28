#!/bin/sh

#  libcopy.sh
#  kitupiikki
#
#  Created by Petri Aarnio on 29/06/2018.
#  

APPCONTENTS=$1/kitupiikki.app/Contents
APPDIR=$APPCONTENTS/MacOS
LIBDIR=$APPCONTENTS/Libraries

mkdir -p $LIBDIR/

#app
install_name_tool -change /usr/local/opt/poppler/lib/libpoppler-qt5.1.dylib @rpath/libpoppler-qt5.dylib $APPDIR/kitupiikki
install_name_tool -change /usr/local/opt/libzip/lib/libzip.5.dylib @rpath/libzip.dylib $APPDIR/kitupiikki

#libpoppler-qt5
cp /usr/local/opt/poppler/lib/libpoppler-qt5.dylib $LIBDIR
chmod +w $LIBDIR/libpoppler-qt5.dylib
install_name_tool -id @rpath/libpoppler-qt5.dylib $LIBDIR/libpoppler-qt5.dylib
install_name_tool -change /usr/local/Cellar/poppler/0.72.0/lib/libpoppler.83.dylib @rpath/libpoppler.dylib $LIBDIR/libpoppler-qt5.dylib
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
install_name_tool -change /usr/local/opt/nss/lib/libnss3.dylib @rpath/libnss.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nss/lib/libnssutil3.dylib @rpath/libnssutil.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nss/lib/libsmime3.dylib @rpath/libsmime.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nss/lib/libssl3.dylib @rpath/libssl.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/openjpeg/lib/libopenjp2.7.dylib @rpath/libopenjp.dylib $LIBDIR/libpoppler.dylib
install_name_tool -change /usr/local/opt/little-cms2/lib/liblcms2.2.dylib @rpath/liblcms.dylib $LIBDIR/libpoppler.dylib
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

#libopenjp
cp /usr/local/opt/openjpeg/lib/libopenjp2.dylib $LIBDIR/libopenjp.dylib
chmod +w $LIBDIR/libopenjp.dylib
install_name_tool -id @rpath/libopenjp.dylib $LIBDIR/libopenjp.dylib

#libpng
cp /usr/local/opt/libpng/lib/libpng.dylib $LIBDIR
chmod +w $LIBDIR/libpng.dylib
install_name_tool -id @rpath/libpng.dylib $LIBDIR/libpng.dylib

#libtiff
cp /usr/local/opt/libtiff/lib/libtiff.dylib $LIBDIR
chmod +w $LIBDIR/libtiff.dylib
install_name_tool -id @rpath/libtiff.dylib $LIBDIR/libtiff.dylib
install_name_tool -change /usr/local/opt/jpeg/lib/libjpeg.9.dylib @rpath/libjpeg.dylib $LIBDIR/libtiff.dylib

#libnss
cp /usr/local/opt/nss/lib/libnss3.dylib $LIBDIR/libnss.dylib
chmod +w $LIBDIR/libnss.dylib
install_name_tool -id @rpath/libnss.dylib $LIBDIR/libnss.dylib
install_name_tool -change /usr/local/Cellar/nss/3.41/lib/libnssutil3.dylib @rpath/libnssutil.dylib $LIBDIR/libnss.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc.dylib $LIBDIR/libnss.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds.dylib $LIBDIR/libnss.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr.dylib $LIBDIR/libnss.dylib

#libnssutil
cp /usr/local/opt/nss/lib/libnssutil3.dylib $LIBDIR/libnssutil.dylib
chmod +w $LIBDIR/libnssutil.dylib
install_name_tool -id @rpath/libnssutil.dylib $LIBDIR/libnssutil.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc.dylib $LIBDIR/libnssutil.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds.dylib $LIBDIR/libnssutil.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr.dylib $LIBDIR/libnssutil.dylib

#libplc
cp /usr/local/opt/nspr/lib/libplc4.dylib $LIBDIR/libplc.dylib
chmod +w $LIBDIR/libplc.dylib
install_name_tool -id @rpath/libplc.dylib $LIBDIR/libplc.dylib
install_name_tool -change /usr/local/Cellar/nspr/4.20/lib/libnspr4.dylib @rpath/libnspr.dylib $LIBDIR/libplc.dylib

#libnspr
cp /usr/local/opt/nspr/lib/libnspr4.dylib $LIBDIR/libnspr.dylib
chmod +w $LIBDIR/libnspr.dylib
install_name_tool -id @rpath/libnspr.dylib $LIBDIR/libnspr.dylib

#libplds
cp /usr/local/opt/nspr/lib/libplds4.dylib $LIBDIR/libplds.dylib
chmod +w $LIBDIR/libplds.dylib
install_name_tool -id @rpath/libplds.dylib $LIBDIR/libplds.dylib
install_name_tool -change /usr/local/Cellar/nspr/4.20/lib/libnspr4.dylib @rpath/libnspr.dylib $LIBDIR/libplds.dylib

#libsmime
cp /usr/local/opt/nss/lib/libsmime3.dylib $LIBDIR/libsmime.dylib
chmod +w $LIBDIR/libsmime.dylib
install_name_tool -id @rpath/libsmime.dylib $LIBDIR/libsmime.dylib
install_name_tool -change /usr/local/Cellar/nss/3.41/lib/libnss3.dylib @rpath/libnss.dylib $LIBDIR/libsmime.dylib
install_name_tool -change /usr/local/Cellar/nss/3.41/lib/libnssutil3.dylib @rpath/libnssutil.dylib $LIBDIR/libsmime.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc.dylib $LIBDIR/libsmime.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds.dylib $LIBDIR/libsmime.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr.dylib $LIBDIR/libsmime.dylib

#libssl
cp /usr/local/opt/nss/lib/libssl3.dylib $LIBDIR/libssl.dylib
chmod +w $LIBDIR/libssl.dylib
install_name_tool -id @rpath/libssl.dylib $LIBDIR/libssl.dylib
install_name_tool -change /usr/local/Cellar/nss/3.41/lib/libnss3.dylib @rpath/libnss.dylib $LIBDIR/libssl.dylib
install_name_tool -change /usr/local/Cellar/nss/3.41/lib/libnssutil3.dylib @rpath/libnssutil.dylib $LIBDIR/libssl.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplc4.dylib @rpath/libplc.dylib $LIBDIR/libssl.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libplds4.dylib @rpath/libplds.dylib $LIBDIR/libssl.dylib
install_name_tool -change /usr/local/opt/nspr/lib/libnspr4.dylib @rpath/libnspr.dylib $LIBDIR/libssl.dylib

#liblcms
cp /usr/local/opt/little-cms2/lib/liblcms2.2.dylib $LIBDIR/liblcms.dylib
chmod +w $LIBDIR/liblcms.dylib
install_name_tool -change /usr/local/opt/little-cms2/lib/liblcms2.2.dylib @rpath/liblcms.dylib $LIBDIR/liblcms.dylib

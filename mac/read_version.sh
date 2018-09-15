#!/bin/sh

#  read_version.sh
#  kitupiikki
#
#  Created by Petri Aarnio on 15/09/2018.
#  

VERSION=`/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' Info.plist`
BUILD=`/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' Info.plist`

echo "#define VERSION_STRING \"$VERSION ($BUILD)\"" > version.h

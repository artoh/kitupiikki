#!/bin/sh

#  read_version.sh
#  kitupiikki
#
#  Created by Petri Aarnio on 15/09/2018.
#  

KITUPIIKKI_VERSIO=`/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' Info.plist`
KITUPIIKKI_BUILD=`/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' Info.plist`

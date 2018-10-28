#!/bin/sh

#  read_version.sh
#  kitupiikki
#
#  Created by Petri Aarnio on 15/09/2018.
#  


VERSIO_H=../kitupiikki/versio.h
KITUPIIKKI_VERSION=`/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' Info.plist`
KITUPIIKKI_BUILD=`/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' Info.plist`

echo 's/KITUPIIKKI_VERSIO ".*"''/KITUPIIKKI_VERSIO "'$KITUPIIKKI_VERSION'"/g'

perl  -i -pe 's/KITUPIIKKI_VERSIO ".*"/KITUPIIKKI_VERSIO "'$KITUPIIKKI_VERSION'"/g' $VERSIO_H
perl  -i -pe 's/KITUPIIKKI_BUILD ".*"/KITUPIIKKI_BUILD "'$KITUPIIKKI_BUILD'"/g' $VERSIO_H

#!/bin/sh

#  read_version.sh
#  kitsas
#
#  Created by Petri Aarnio on 15/09/2018.
#  


VERSIO_H=../kitsas/versio.h
#VERSION=`/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' Info.plist`
#BUILD=`/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' Info.plist`

echo 's/KITSAS_VERSIO ".*"''/KITSAS_VERSIO "'$CURRENT_MARKETING_VERSION'"/g'

perl  -i -pe 's/KITSAS_VERSIO ".*"/KITSAS_VERSIO "'$MARKETING_VERSION'"/g' $VERSIO_H
perl  -i -pe 's/KITSAS_BUILD  ".*"/KITSAS_BUILD "'$CURRENT_PROJECT_VERSION'"/g' $VERSIO_H

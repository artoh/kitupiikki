#!/bin/sh

#  strip_debug_libs.sh
#  kitupiikki
#
#  Created by Petri Aarnio on 08/09/2018.
#  

APPCONTENTS=$1/kitupiikki.app/Contents
FRAMEWORKS_DIR=$APPCONTENTS/Frameworks

find $FRAMEWORKS_DIR -name '*_debug*' -exec rm {} \;

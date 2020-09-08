#!/bin/sh

#  strip_debug_libs.sh
#  kitsas
#
#  Created by Petri Aarnio on 08/09/2018.
#  

APPCONTENTS=$1/kitsas.app/Contents
FRAMEWORKS_DIR=$APPCONTENTS/Frameworks

find $FRAMEWORKS_DIR -name '*_debug*' -exec rm {} \;
find $FRAMEWORKS_DIR -name '*.prl' -exec rm {} \;

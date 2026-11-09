#!/bin/sh

#  update_project.sh
#  kitsas
#
#  Created by Petri Aarnio on 14/02/2019.
#

cd "$(dirname "$0")"

# qmake caches the macOS SDK version in .qmake.stash; remove stale entries
# after Xcode upgrades (otherwise you get "building against version 12.1").
rm -f .qmake.stash

~/Qt/6.11.1/macos/bin/qmake -spec macx-xcode ../kitsas/kitsas.pro
./qt_preprocess.sh

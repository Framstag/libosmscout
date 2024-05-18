#!/bin/sh
set -e

if [ $# -ge 1 ] ; then
  REPO="$1"
else
  REPO="https://github.com/Framstag/libosmscout.git"
fi

if [ $# -ge 2 ] ; then
  BRANCH="$2"
else
  BRANCH="master"
fi

git clone -b "$BRANCH" "$REPO" libosmscout

export LANG=en_US.utf8
cd libosmscout
mkdir debug
PATH=/usr/lib/qt6/bin:/usr/lib/qt6/libexec:$PATH meson setup --buildtype debugoptimized --unity on --wrap-mode=nofallback -D qtVersion=6 debug
ninja -C debug
meson test -C debug --print-errorlogs


#!/bin/sh
set -e

if [ $# -ge 1 ] ; then
  REPO="$1"
else
  REPO="git://git.code.sf.net/p/libosmscout/code"
fi

if [ $# -ge 2 ] ; then
  BRANCH="$2"
else
  BRANCH="master"
fi

git clone -b "$BRANCH" "$REPO" libosmscout

env

cd libosmscout
meson debug
cd debug

# workaround for meson 0.44 issue https://github.com/mesonbuild/meson/issues/2763
ln -s . ../OSMScout2/OSMScout2
ln -s . ../StyleEditor/StyleEditor

ninja
ninja test

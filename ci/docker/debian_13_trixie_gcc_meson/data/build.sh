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

env

cd libosmscout
meson debug
cd debug

ninja
ninja test

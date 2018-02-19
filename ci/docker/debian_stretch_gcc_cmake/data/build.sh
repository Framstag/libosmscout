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
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=DEBUG -DOSMSCOUT_BUILD_BINDING_JAVA=OFF ..
make -j `nproc`
make test


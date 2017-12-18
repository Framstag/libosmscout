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
export CC=clang
export CXX=clang++

cd libosmscout
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=DEBUG -DOSMSCOUT_BUILD_BINDING_JAVA=OFF ..
make -j `nproc`
ctest --output-on-failure


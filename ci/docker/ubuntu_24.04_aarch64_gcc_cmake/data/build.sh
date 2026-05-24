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
mkdir build
cd build
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CROSSCOMPILING_EMULATOR=/usr/bin/qemu-aarch64 \
    -DCMAKE_PREFIX_PATH="/usr/aarch64-linux-gnu/usr/;/usr/aarch64-linux-gnu/" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath,/usr/aarch64-linux-gnu/lib/aarch64-linux-gnu/" \
    ..

make -j $(nproc) install
ctest --output-on-failure


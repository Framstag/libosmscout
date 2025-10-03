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
  -DCMAKE_BUILD_TYPE=DEBUG \
  -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fsanitize=memory" \
  -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -fsanitize=memory" \
  -DCMAKE_C_COMPILER=/usr/bin/clang-20 \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++-20 \
  ..
make -j $(nproc) install
ctest --output-on-failure

